/* 32bit ELF object functions
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <types.h>
#include <console.h>
#include <macros.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <linker/elf32.h>
#include <lib/string.h>

static inline uint32_t elf32_symbol_name_hash( const char* name ) {
    uint32_t g;
    uint32_t hash = 0;

    while ( *name != 0 ) {
        hash = ( hash << 4 ) + *name++;
        g = hash & 0xF0000000;

        if ( g != 0 ) {
            hash ^= g >> 24;
        }

        hash &= ~g;
    }

    return hash;
}

int elf32_load_and_validate_header( elf32_image_info_t* info, binary_loader_t* loader, uint16_t type ) {
    if ( loader->read( loader->private, &info->header, 0, sizeof( elf_header_t ) ) != sizeof( elf_header_t ) ) {
        return -EIO;
    }

    if ( !elf_check_header( &info->header, ELF_CLASS_32, ELF_DATA_2_LSB, type, ELF_MACHINE_386 ) ) {
        return -EINVAL;
    }

    return 0;
}

int elf32_load_section_headers( elf32_image_info_t* info, binary_loader_t* loader ) {
    int error;
    int sh_table_size;
    elf_section_header_t* shstrtab;

    /* Load the section headers */

    if ( info->header.shnum == 0 ) {
        return -EINVAL;
    }

    sh_table_size = sizeof( elf_section_header_t ) * info->header.shnum;
    info->section_headers = ( elf_section_header_t* )kmalloc( sh_table_size );

    if ( info->section_headers == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( loader->read( loader->private, info->section_headers, info->header.shoff, sh_table_size ) != sh_table_size ) {
        error = -EIO;
        goto error2;
    }

    /* Load the shstrtab section */

    shstrtab = &info->section_headers[ info->header.shstrndx ];

    if ( shstrtab->size == 0 ) {
        error = -EINVAL;
        goto error2;
    }

    info->sh_string_table = ( char* )kmalloc( shstrtab->size );

    if ( info->sh_string_table == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    if ( loader->read( loader->private, info->sh_string_table, shstrtab->offset, shstrtab->size ) != shstrtab->size ) {
        error = -EIO;
        goto error3;
    }

    return 0;

 error3:
    kfree( info->sh_string_table );
    info->sh_string_table = NULL;

 error2:
    kfree( info->section_headers );
    info->section_headers = NULL;

 error1:
    return error;
}

static int elf32_load_strtab_section( elf32_image_info_t* info, binary_loader_t* loader, elf_section_header_t* strtab ) {
    int error;

    info->string_table = ( char* )kmalloc( strtab->size );

    if ( info->string_table == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( loader->read( loader->private, ( void* )info->string_table, strtab->offset, strtab->size ) != strtab->size ) {
        error = -EIO;
        goto error2;
    }

    return 0;

 error2:
    kfree( info->string_table );
    info->string_table = NULL;

 error1:
    return error;
}

static int elf32_load_symtab_section( elf32_image_info_t* info, binary_loader_t* loader, elf_section_header_t* symtab ) {
    int error;

    info->symbol_table = ( elf_symbol_t* )kmalloc( symtab->size );

    if ( info->symbol_table == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( loader->read( loader->private, info->symbol_table, symtab->offset, symtab->size ) != symtab->size ) {
        error = -EIO;
        goto error2;
    }

    info->symbol_count = symtab->size / sizeof( elf_symbol_t );

    return 0;

 error2:
    kfree( info->symbol_table );
    info->symbol_table = 0;

 error1:
    return error;
}

static int elf32_load_dynamic_section( elf32_image_info_t* info, binary_loader_t* loader,
                                       elf_section_header_t* dynamic_section ) {
    int error;
    uint32_t i;
    uint32_t dynamic_count;
    elf_dynamic_t* dynamic;
    elf_dynamic_t* dynamic_table;

    uint32_t rel_address = 0;
    uint32_t rel_size = 0;
    uint32_t pltrel_address = 0;
    uint32_t pltrel_size = 0;

    dynamic_table = ( elf_dynamic_t* )kmalloc( dynamic_section->size );

    if ( dynamic_table == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( loader->read( loader->private, dynamic_table, dynamic_section->offset,
                       dynamic_section->size ) != dynamic_section->size ) {
        error = -EIO;
        goto error2;
    }

    dynamic_count = dynamic_section->size / sizeof( elf_dynamic_t );

    for ( i = 0, dynamic = &dynamic_table[ 0 ]; i < dynamic_count; i++, dynamic++ ) {
        switch ( dynamic->tag ) {
            case DT_REL :
                rel_address = dynamic->value;
                break;

            case DT_RELSZ :
                rel_size = dynamic->value;
                break;

            case DT_JMPREL :
                pltrel_address = dynamic->value;
                break;

            case DT_PLTRELSZ :
                pltrel_size = dynamic->value;
                break;

            case DT_NEEDED :
                info->needed_count++;
                break;
        }
    }

    /* Build the table of the needed subimages if it's not empty. */

    if ( info->needed_count > 0 ) {
        uint32_t needed_index = 0;

        info->needed_table = ( char** )kmalloc( sizeof( char* ) * info->needed_count );

        if ( info->needed_table == NULL ) {
            goto error2;
        }

        for ( i = 0, dynamic = &dynamic_table[0]; i < dynamic_count; i++, dynamic++ ) {
            if ( dynamic->tag == DT_NEEDED ) {
                info->needed_table[ needed_index++ ] = info->dyn_string_table + dynamic->value;
            }
        }
    }

    kfree( dynamic_table );

    if ( ( rel_size > 0 ) ||
         ( pltrel_size > 0 ) ) {
        uint32_t reloc_size = rel_size + pltrel_size;

        info->reloc_count = reloc_size / sizeof( elf_reloc_t );
        info->reloc_table = ( elf_reloc_t* )kmalloc( reloc_size );

        if ( info->reloc_table == NULL ) {
            error = -ENOMEM;
            goto error1;
        }

        if ( rel_size > 0 ) {
            error = loader->read(
                loader->private,
                ( void* )info->reloc_table,
                rel_address - info->virtual_address,
                rel_size
            );

            if ( __unlikely( error != rel_size ) ) {
                error = -EIO;
                goto error3;
            }
        }

        if ( pltrel_size > 0 ) {
            error = loader->read(
                loader->private,
                ( char* )info->reloc_table + rel_size,
                pltrel_address - info->virtual_address,
                pltrel_size
            );

            if ( __unlikely( error != pltrel_size ) ) {
                error = -EIO;
                goto error3;
            }
        }
    }

    return 0;

 error3:
    if ( info->needed_count > 0 ) {
        info->needed_count = 0;
        kfree( info->needed_table );
        info->needed_table = NULL;
    }

    info->reloc_count = 0;
    kfree( info->reloc_table );
    info->reloc_table = NULL;

    return error;

error2:
    kfree( dynamic_table );

error1:
    return error;
}

static int elf32_load_dynstr_section( elf32_image_info_t* info, binary_loader_t* loader, elf_section_header_t* dynstr ) {
    int error;

    info->dyn_string_table = ( char* )kmalloc( dynstr->size );

    if ( info->dyn_string_table == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( loader->read( loader->private, ( void* )info->dyn_string_table, dynstr->offset, dynstr->size ) != dynstr->size ) {
        error = -EIO;
        goto error2;
    }

    return 0;

 error2:
    kfree( info->dyn_string_table );
    info->dyn_string_table = NULL;

 error1:
    return error;
}

static int elf32_load_dynsym_section( elf32_image_info_t* info, binary_loader_t* loader, elf_section_header_t* dynsym ) {
    int error;
    uint32_t i;
    uint32_t symbol_count;
    elf_symbol_t* elf_symbol;
    elf_symbol_t* elf_symbols;
    my_elf_symbol_t* my_elf_symbol;

    symbol_count = dynsym->size / sizeof( elf_symbol_t );

    elf_symbols = ( elf_symbol_t* )kmalloc( dynsym->size );

    if ( elf_symbols == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( loader->read( loader->private, elf_symbols, dynsym->offset, dynsym->size ) != dynsym->size ) {
        error = -EIO;
        goto error2;
    }

    info->dyn_symbol_table = ( my_elf_symbol_t* )kmalloc( sizeof( my_elf_symbol_t ) * symbol_count );

    if ( info->dyn_symbol_table == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    for ( i = 0, elf_symbol = &elf_symbols[0], my_elf_symbol = &info->dyn_symbol_table[0];
          i < symbol_count;
          i++, elf_symbol++, my_elf_symbol++ ) {
        my_elf_symbol->name = info->dyn_string_table + elf_symbol->name;
        my_elf_symbol->address = elf_symbol->value;
        my_elf_symbol->size = elf_symbol->size;
        my_elf_symbol->info = elf_symbol->info;
        my_elf_symbol->section = elf_symbol->shndx;
    }

    kfree( elf_symbols );

    info->dyn_symbol_count = symbol_count;

    return 0;

 error2:
    kfree( elf_symbols );

 error1:
    return error;
}

static int elf32_load_hash_section( elf32_image_info_t* info, binary_loader_t* loader, elf_section_header_t* hashtab ) {
    int error;

    info->symhash = ( elf_hashtable_t* )kmalloc( hashtab->size );

    if ( info->symhash == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( loader->read( loader->private, info->symhash, hashtab->offset, hashtab->size ) != hashtab->size ) {
        error = -EIO;
        goto error2;
    }

    info->sym_bucket = ( uint32_t* )( info->symhash + 1 );
    info->sym_chain = info->sym_bucket + info->symhash->bucket_cnt;

    return 0;

 error2:
    kfree( info->symhash );
    info->symhash = NULL;

 error1:
    return error;
}

int elf32_parse_section_headers( elf32_image_info_t* info, binary_loader_t* loader ) {
    int error;
    uint32_t i;
    elf_section_header_t* current;
    elf_section_header_t* strtab;
    elf_section_header_t* symtab;
    elf_section_header_t* dynamic;
    elf_section_header_t* dynsym;
    elf_section_header_t* dynstr;
    elf_section_header_t* hashtab;

    strtab = NULL;
    symtab = NULL;
    dynamic = NULL;
    dynsym = NULL;
    dynstr = NULL;
    hashtab = NULL;

    for ( i = 0, current = &info->section_headers[0]; i < info->header.shnum; i++, current++ ) {
        switch ( current->type ) {
            case SHT_STRTAB : {
                char* section_name;

                section_name = info->sh_string_table + current->name;

                if ( strcmp( section_name, ".strtab" ) == 0 ) {
                    strtab = current;
                } else if ( strcmp( section_name, ".dynstr" ) == 0 ) {
                    dynstr = current;
                }

                break;
            }

            case SHT_SYMTAB :
                symtab = current;
                break;

            case SHT_DYNAMIC :
                dynamic = current;
                break;

            case SHT_DYNSYM :
                dynsym = current;
                break;

            case SHT_HASH :
                hashtab = current;
                break;
        }
    }

    if ( ( strtab == NULL ) ||
         ( symtab == NULL ) ) {
        return -EINVAL;
    }

    error = elf32_load_strtab_section( info, loader, strtab );

    if ( error < 0 ) {
        return error;
    }

    error = elf32_load_symtab_section( info, loader, symtab );

    if ( error < 0 ) {
        return error;
    }

    if ( dynstr != NULL ) {
        error = elf32_load_dynstr_section( info, loader, dynstr );

        if ( error < 0 ) {
            return error;
        }
    }

    if ( dynamic != NULL ) {
        error = elf32_load_dynamic_section( info, loader, dynamic );

        if ( error < 0 ) {
            return error;
        }
    }

    if ( dynsym != NULL ) {
        error = elf32_load_dynsym_section( info, loader, dynsym );

        if ( error < 0 ) {
            return error;
        }
    }

    if ( hashtab != NULL ) {
        error = elf32_load_hash_section( info, loader, hashtab );

        if ( error < 0 ) {
            return error;
        }
    }

    return 0;
}

int elf32_free_section_headers( elf32_image_info_t* info ) {
    kfree( info->sh_string_table );
    info->sh_string_table = NULL;
    kfree( info->section_headers );
    info->section_headers = NULL;

    return 0;
}

int elf32_free_reloc_table( elf32_image_info_t* info ) {
    kfree( info->reloc_table );
    info->reloc_table = NULL;
    info->reloc_count = 0;

    return 0;
}

my_elf_symbol_t* elf32_get_symbol( elf32_image_info_t* info, const char* name ) {
    uint32_t hash = elf32_symbol_name_hash( name );
    uint32_t index = info->sym_bucket[ hash % info->symhash->bucket_cnt ];

    do {
        my_elf_symbol_t* symbol;

        symbol = &info->dyn_symbol_table[ index ];

        if ( ( symbol->section != SHN_UNDEF ) &&
             ( strcmp( symbol->name, name ) == 0 ) ) {
            return symbol;
        }

        index = info->sym_chain[ index ];
    } while ( index != STN_UNDEF );

    return NULL;
}

elf_symbol_t* elf32_get_symbol2( elf32_image_info_t* info, const char* name ) {
    uint32_t i;
    elf_symbol_t* symbol;

    symbol = info->symbol_table;

    for ( i = 0; i < info->symbol_count; i++, symbol++ ) {
        if ( strcmp( info->string_table + symbol->name, name ) == 0 ) {
            return symbol;
        }
    }

    return NULL;
}

int elf32_get_symbol_info( elf32_image_info_t* info, ptr_t address, symbol_info_t* symbol_info ) {
    uint32_t i;
    elf_symbol_t* symbol;

    /* Image without symbols?! */
    if (info->symbol_count == 0) {
        return -ENOENT;
    }

    symbol_info->name = NULL;
    symbol_info->address = 0;

    symbol = info->symbol_table;

    for ( i = 0; i < info->symbol_count; i++, symbol++ ) {
        char* name;

        if ( symbol->value > address ) {
            continue;
        }

        name = info->string_table + symbol->name;

        if ( symbol_info->name == NULL ) {
            symbol_info->name = name;
            symbol_info->address = symbol->value;
        } else {
            int last_diff;
            int curr_diff;

            last_diff = address - symbol_info->address;
            curr_diff = address - symbol->value;

            if ( curr_diff <= last_diff ) {
                symbol_info->name = name;
                symbol_info->address = symbol->value;
            }
        }
    }

    if ( symbol_info->name == NULL ) {
        return -ENOENT;
    }

    return 0;
}

int elf32_init_image_info( elf32_image_info_t* info, ptr_t virtual_address ) {
    memset( info, 0, sizeof( elf32_image_info_t ) );

    info->virtual_address = virtual_address;

    return 0;
}

int elf32_destroy_image_info( elf32_image_info_t* info ) {
    kfree( info->section_headers );
    info->section_headers = NULL;

    kfree( info->string_table );
    info->string_table = NULL;
    kfree( info->sh_string_table );
    info->sh_string_table = NULL;
    kfree( info->dyn_string_table );
    info->dyn_string_table = NULL;

    kfree( info->symbol_table );
    info->symbol_table = NULL;
    info->symbol_count = 0;

    kfree( info->dyn_symbol_table );
    info->dyn_symbol_table = NULL;
    info->dyn_symbol_count = 0;

    kfree( info->reloc_table );
    info->reloc_table = NULL;
    info->reloc_count = 0;

    kfree( info->needed_table );
    info->needed_table = NULL;
    info->needed_count = 0;

    kfree( info->symhash );
    info->symhash = NULL;
    info->sym_bucket = NULL;
    info->sym_chain = NULL;

    return 0;
}
