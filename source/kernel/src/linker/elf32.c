/* 32bit ELF object functions
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
#include <multiboot.h>
#include <console.h>
#include <macros.h>
#include <symbols.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <linker/elf32.h>
#include <lib/string.h>

extern multiboot_header_t mb_header;

int elf32_load_and_validate_header( elf32_image_info_t* info, binary_loader_t* loader, uint16_t type ) {
    int error;

    error = loader->read(
        loader->private,
        ( void* )&info->header,
        0x0,
        sizeof( elf_header_t )
    );

    if ( __unlikely( error != sizeof( elf_header_t ) ) ) {
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

    sh_table_size = sizeof( elf_section_header_t ) * info->header.shnum;

    info->section_headers = ( elf_section_header_t* )kmalloc( sh_table_size );

    if ( info->section_headers == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    error = loader->read(
        loader->private,
        info->section_headers,
        info->header.shoff,
        sh_table_size
    );

    if ( __unlikely( error != sh_table_size ) ) {
        error = -EIO;
        goto error2;
    }

    /* Load the shstrtab section */

    shstrtab = &info->section_headers[ info->header.shstrndx ];

    info->sh_string_table = ( char* )kmalloc( shstrtab->size );

    if ( info->sh_string_table == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    error = loader->read(
        loader->private,
        info->sh_string_table,
        shstrtab->offset,
        shstrtab->size
    );

    if ( __unlikely( error != shstrtab->size ) ) {
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

    error = loader->read(
        loader->private,
        ( void* )info->string_table,
        strtab->offset,
        strtab->size
    );

    if ( __unlikely( error != strtab->size ) ) {
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
    uint32_t i;
    uint32_t symbol_count;
    elf_symbol_t* elf_symbol;
    elf_symbol_t* elf_symbols;
    my_elf_symbol_t* my_elf_symbol;

    symbol_count = symtab->size / sizeof( elf_symbol_t );

    elf_symbols = ( elf_symbol_t* )kmalloc( symtab->size );

    if ( elf_symbols == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    error = loader->read(
        loader->private,
        elf_symbols,
        symtab->offset,
        symtab->size
    );

    if ( __unlikely( error != symtab->size ) ) {
        error = -EIO;
        goto error2;
    }

    info->symbol_table = ( my_elf_symbol_t* )kmalloc( sizeof( my_elf_symbol_t ) * symbol_count );

    if ( info->symbol_table == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    for ( i = 0, elf_symbol = &elf_symbols[ 0 ], my_elf_symbol = &info->symbol_table[ 0 ];
          i < symbol_count;
          i++, elf_symbol++, my_elf_symbol++ ) {
        my_elf_symbol->name = info->string_table + elf_symbol->name;
        my_elf_symbol->address = elf_symbol->value;
        my_elf_symbol->info = elf_symbol->info;

        hashtable_add( &info->symbol_hash_table, ( hashitem_t* )my_elf_symbol );
    }

    kfree( elf_symbols );

    info->symbol_count = symbol_count;

    return 0;

 error2:
    kfree( elf_symbols );

 error1:
    return error;
}

static int elf32_load_dynamic_section( elf32_image_info_t* info, binary_loader_t* loader, elf_section_header_t* dynamic_section ) {
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

    error = loader->read(
        loader->private,
        dynamic_table,
        dynamic_section->offset,
        dynamic_section->size
    );

    if ( __unlikely( error != dynamic_section->size ) ) {
        error = -EIO;
        goto error2;
    }

    dynamic_count = dynamic_section->size / sizeof( elf_dynamic_t );

    for ( i = 0, dynamic = &dynamic_table[ 0 ]; i < dynamic_count; i++, dynamic++ ) {
        switch ( dynamic->tag ) {
            case DYN_REL :
                rel_address = dynamic->value;
                break;

            case DYN_RELSZ :
                rel_size = dynamic->value;
                break;

            case DYN_JMPREL :
                pltrel_address = dynamic->value;
                break;

            case DYN_PLTRELSZ :
                pltrel_size = dynamic->value;
                break;
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
                rel_address,
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
                pltrel_address,
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

    error = loader->read(
        loader->private,
        ( void* )info->dyn_string_table,
        dynstr->offset,
        dynstr->size
    );

    if ( __unlikely( error != dynstr->size ) ) {
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

    error = loader->read(
        loader->private,
        elf_symbols,
        dynsym->offset,
        dynsym->size
    );

    if ( __unlikely( error != dynsym->size ) ) {
        error = -EIO;
        goto error2;
    }

    info->dyn_symbol_table = ( my_elf_symbol_t* )kmalloc( sizeof( my_elf_symbol_t ) * symbol_count );

    if ( info->dyn_symbol_table == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    for ( i = 0, elf_symbol = &elf_symbols[ 0 ], my_elf_symbol = &info->dyn_symbol_table[ 0 ];
          i < symbol_count;
          i++, elf_symbol++, my_elf_symbol++ ) {
        my_elf_symbol->name = info->dyn_string_table + elf_symbol->name;
        my_elf_symbol->address = elf_symbol->value;
        my_elf_symbol->info = elf_symbol->info;
    }

    kfree( elf_symbols );

    info->dyn_symbol_count = symbol_count;

    return 0;

error2:
    kfree( elf_symbols );

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

    strtab = NULL;
    symtab = NULL;
    dynamic = NULL;
    dynsym = NULL;
    dynstr = NULL;

    for ( i = 0, current = &info->section_headers[ i ]; i < info->header.shnum; i++, current++ ) {
        switch ( current->type ) {
            case SECTION_STRTAB : {
                char* section_name;

                section_name = info->sh_string_table + current->name;

                if ( strcmp( section_name, ".strtab" ) == 0 ) {
                    strtab = current;
                } else if ( strcmp( section_name, ".dynstr" ) == 0 ) {
                    dynstr = current;
                }

                break;
            }

            case SECTION_SYMTAB :
                symtab = current;
                break;

            case SECTION_DYNAMIC :
                dynamic = current;
                break;

            case SECTION_DYNSYM :
                dynsym = current;
                break;
        }
    }

    if ( ( strtab == NULL ) ||
         ( symtab == NULL ) ) {
        return -EINVAL;
    }

    error = elf32_load_strtab_section( info, loader, strtab );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    error = elf32_load_symtab_section( info, loader, symtab );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    if ( dynamic != NULL ) {
        error = elf32_load_dynamic_section( info, loader, dynamic );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }
    }

    if ( dynstr != NULL ) {
        error = elf32_load_dynstr_section( info, loader, dynstr );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }
    }

    if ( dynsym != NULL ) {
        error = elf32_load_dynsym_section( info, loader, dynsym );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }
    }

    return 0;
}

my_elf_symbol_t* elf32_get_symbol( elf32_image_info_t* info, const char* name ) {
    return ( my_elf_symbol_t* )hashtable_get( &info->symbol_hash_table, ( const void* )name );
}

int elf32_get_symbol_info( elf32_image_info_t* info, ptr_t address, symbol_info_t* symbol_info ) {
    uint32_t i;
    my_elf_symbol_t* symbol;

    /* Image without symbols?! */

    if ( info->symbol_count == 0 ) {
        return false;
    }

    /* Make sure that the specified address is inside of this module */

    symbol_info->name = NULL;
    symbol_info->address = 0;

    symbol = info->symbol_table;

    for ( i = 0; i < info->symbol_count; i++, symbol++ ) {
        if ( symbol->address > address ) {
            continue;
        }

        if ( symbol_info->name == NULL ) {
            symbol_info->name = symbol->name;
            symbol_info->address = symbol->address;
        } else {
            int last_diff;
            int curr_diff;

            last_diff = address - symbol_info->address;
            curr_diff = address - symbol->address;

            if ( curr_diff < last_diff ) {
                symbol_info->name = symbol->name;
                symbol_info->address = symbol->address;
            }
        }
    }

    if ( symbol_info->name == NULL ) {
        return -ENOENT;
    }

    return 0;
}

static void* symbol_key( hashitem_t* item ) {
    my_elf_symbol_t* symbol;

    symbol = ( my_elf_symbol_t* )item;

    return ( void* )symbol->name;
}

int elf32_init_image_info( elf32_image_info_t* info ) {
    int error;

    error = init_hashtable(
        &info->symbol_hash_table,
        256,
        symbol_key,
        hash_str,
        compare_str
    );

    if ( error < 0 ) {
        return error;
    }

    info->section_headers = NULL;
    info->string_table = NULL;
    info->sh_string_table = NULL;
    info->dyn_string_table = NULL;
    info->symbol_count = 0;
    info->symbol_table = NULL;
    info->dyn_symbol_count = 0;
    info->dyn_symbol_table = NULL;
    info->reloc_count = 0;
    info->reloc_table = NULL;

    return 0;
}

int elf32_destroy_image_info( elf32_image_info_t* info ) {
    destroy_hashtable( &info->symbol_hash_table );

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

    return 0;
}

__init int init_elf32_kernel_symbols( void ) {
    uint32_t i;
    elf_symbol_t* symbol;
    const char* string_table = NULL;
    const char* tmp_string_table;
    elf_section_header_t* strtab;
    elf_section_header_t* symtab;

    /* Do we have ELF information from the multiboot header? */

    if ( ( mb_header.flags & MB_FLAG_ELF_SYM_INFO ) == 0 ) {
        kprintf( WARNING, "ELF symbol information not provided for the kernel!\n" );

        return 0;
    }

    /* Get the string table */

    strtab = ( elf_section_header_t* )( mb_header.elf_info.addr + mb_header.elf_info.shndx * mb_header.elf_info.size );
    tmp_string_table = ( const char* )strtab->address;

    /* Get the symbol table */

    symtab = NULL;

    for ( i = 1; i < mb_header.elf_info.num; i++ ) {
        const char* section_name;
        elf_section_header_t* tmp;

        tmp = ( elf_section_header_t* )( mb_header.elf_info.addr + i * mb_header.elf_info.size );
        section_name = tmp_string_table + tmp->name;

        if ( strcmp( section_name, ".symtab" ) == 0 ) {
            symtab = tmp;
        } else if ( strcmp( section_name, ".strtab" ) == 0 ) {
            string_table = ( const char* )tmp->address;
        }
    }

    if ( ( symtab == NULL ) ||
         ( string_table == NULL ) ) {
        kprintf( WARNING, "ELF symbol table not found for the kernel!\n" );

        return 0;
    }

    for ( i = 0, symbol = ( elf_symbol_t* )symtab->address; i < ( symtab->size / sizeof( elf_symbol_t ) ); i++, symbol++ ) {
        switch ( ELF32_ST_TYPE( symbol->info ) ) {
            case STT_FUNC :
                add_kernel_symbol(
                    string_table + symbol->name,
                    symbol->value
                );

                break;
        }
    }

    return 0;
}
