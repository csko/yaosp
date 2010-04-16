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
#include <multiboot.h>
#include <console.h>
#include <macros.h>
#include <symbols.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
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

    if ( sh_table_size == 0 ) {
        return -EINVAL;
    }

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

    if ( shstrtab->size == 0 ) {
        error = -EINVAL;
        goto error2;
    }

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
        my_elf_symbol->size = elf_symbol->size;
        my_elf_symbol->info = elf_symbol->info;
        my_elf_symbol->section = elf_symbol->shndx;

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

            case DYN_NEEDED :
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

        for ( i = 0, dynamic = &dynamic_table[ 0 ]; i < dynamic_count; i++, dynamic++ ) {
            if ( dynamic->tag == DYN_NEEDED ) {
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

    return 0;
}

int elf32_free_section_headers( elf32_image_info_t* info ) {
    kfree( info->sh_string_table );
    info->sh_string_table = NULL;
    kfree( info->section_headers );
    info->section_headers = NULL;

    return 0;
}

my_elf_symbol_t* elf32_get_symbol( elf32_image_info_t* info, const char* name ) {
    my_elf_symbol_t* symbol;

    symbol = ( my_elf_symbol_t* )hashtable_get( &info->symbol_hash_table, ( const void* )name );

    if ( ( symbol == NULL ) ||
         ( symbol->section == 0 /* undefined */ ) ) {
        return NULL;
    }

    return symbol;
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

int elf32_init_image_info( elf32_image_info_t* info, ptr_t virtual_address ) {
    int error;

    memset( info, 0, sizeof( elf32_image_info_t ) );

    error = init_hashtable(
        &info->symbol_hash_table, 256,
        symbol_key, hash_str, compare_str
    );

    if ( error < 0 ) {
        return error;
    }

    info->virtual_address = virtual_address;

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

static binary_loader_t* elf32_get_image_loader( char* filename ) {
    int fd;
    char path[256];

    snprintf( path, sizeof(path), "/yaosp/system/lib/%s", filename );

    fd = sys_open( path, O_RDONLY );

    if ( fd < 0 ) {
        return NULL;
    }

    return get_app_binary_loader( filename, fd );
}

static int elf32_image_map( elf32_image_t* image, binary_loader_t* loader, elf_binary_type_t type ) {
    uint32_t i;
    elf_section_header_t* section_header;

    bool text_found = false;
    uint32_t text_start = 0;
    uint32_t text_end = 0;
    uint32_t text_size;
    uint32_t text_offset = 0;

    bool data_found = false;
    uint32_t data_start = 0;
    uint32_t data_end = 0;
    uint32_t data_size;
    uint32_t data_offset = 0;

    uint32_t bss_end = 0;
    uint32_t data_size_with_bss;

    for ( i = 0; i < image->info.header.shnum; i++ ) {
        section_header = &image->info.section_headers[i];

        /* Check if the current section occupies memory during execution */

        if ( section_header->flags & SF_ALLOC ) {
            if ( section_header->flags & SF_WRITE ) {
                if ( !data_found ) {
                    data_found = true;
                    data_start = section_header->address;
                    data_offset = section_header->offset;
                }

                switch ( section_header->type ) {
                    case SECTION_NOBITS :
                        bss_end = section_header->address + section_header->size - 1;
                        break;

                    default :
                        data_end = section_header->address + section_header->size - 1;
                        bss_end = data_end;
                        break;
                }
            } else {
                if ( !text_found ) {
                    text_found = true;
                    text_start = section_header->address;
                    text_offset = section_header->offset;
                }

                text_end = section_header->address + section_header->size - 1;
            }
        }
    }

    if ( !text_found ) {
        return -EINVAL;
    }

    text_offset -= ( text_start & ~PAGE_MASK );
    text_start &= PAGE_MASK;
    text_size = text_end - text_start + 1;

    data_offset -= ( data_start & ~PAGE_MASK );
    data_start &= PAGE_MASK;
    data_size = data_end - data_start + 1;
    data_size_with_bss = bss_end - data_start + 1;

    if ( ( type == ELF_APPLICATION ) ||
         ( type == ELF_LIBRARY ) ) {
        char name[128];
        uint32_t flags;

        snprintf( name, sizeof(name), "%s_ro", loader->get_name( loader->private ) );
        flags = REGION_READ | REGION_EXECUTE;

        if ( type == ELF_LIBRARY ) {
            flags |= REGION_CALL_FROM_USERSPACE; /* todo */
        }

        image->text_region = memory_region_create(
            name, PAGE_ALIGN( text_size ), flags
        );

        if ( image->text_region == NULL ) {
            return -1;
        }

        memory_region_map_to_file(
            image->text_region, loader->get_fd( loader->private ),
            text_offset, text_size
        );
    } else if ( type == ELF_KERNEL_MODULE ) {
        /* todo */
    }

    if ( data_found > 0 ) {
        if ( ( type == ELF_APPLICATION ) ||
             ( type == ELF_LIBRARY ) ) {
            char name[128];
            uint32_t flags;

            snprintf( name, sizeof(name), "%s_rw", loader->get_name( loader->private ) );
            flags = REGION_READ | REGION_WRITE;

            if ( type == ELF_LIBRARY ) {
                flags |= REGION_CALL_FROM_USERSPACE; /* todo */
            }

            image->data_region = memory_region_create(
                name, PAGE_ALIGN( data_size_with_bss ), flags
            );

            if ( image->data_region == NULL ) {
                /* TODO: delete text region */
                return -1;
            }

            if ( ( data_end != 0 ) && ( data_size > 0 ) ) {
                memory_region_map_to_file(
                    image->data_region, loader->get_fd( loader->private ),
                    data_offset, data_size
                );
            } else {
                kprintf( WARNING, "%s(): nothing has been mapped to data region!\n", __FUNCTION__ );
            }
        } else if ( type == ELF_KERNEL_MODULE ) {
            /* todo */
        }
    }

    return 0;
}

int elf32_image_load( elf32_image_t* image, binary_loader_t* loader, ptr_t virtual_address, elf_binary_type_t type ) {
    int error;
    uint32_t i;
    uint16_t elf_type;

    switch ( type ) {
        case ELF_KERNEL_MODULE : elf_type = ELF_TYPE_DYN; break;
        case ELF_APPLICATION : elf_type = ELF_TYPE_EXEC; break;
        case ELF_LIBRARY : elf_type = ELF_TYPE_DYN; break;
        default : return -EINVAL;
    }

    memset( image, 0, sizeof( elf32_image_t ) );

    error = elf32_init_image_info( &image->info, virtual_address );

    if ( error != 0 ) {
        goto error1;
    }

    error = elf32_load_and_validate_header( &image->info, loader, elf_type );

    if ( error != 0 ) {
        goto error2;
    }

    error = elf32_load_section_headers( &image->info, loader );

    if ( error != 0 ) {
        goto error2;
    }

    error = elf32_parse_section_headers( &image->info, loader );

    if ( error != 0 ) {
        goto error2;
    }

    error = elf32_image_map( image, loader, type );

    if ( error != 0 ) {
        goto error2;
    }

    /* Load the required subimages. */

    if ( image->info.needed_count > 0 ) {
        image->subimages = ( elf32_image_t* )kmalloc( sizeof( elf32_image_t ) * image->info.needed_count );

        if ( image->subimages == NULL ) {
            goto error2;
        }

        for ( i = 0; i < image->info.needed_count; i++ ) {
            int fd;
            binary_loader_t* img_loader = elf32_get_image_loader( image->info.needed_table[i] );

            if ( img_loader == NULL ) {
                kprintf( WARNING, "%s(): unable to find binary loader for %s.\n",
                         __FUNCTION__, image->info.needed_table[i] );
                error = -EINVAL;
                goto error2; /* todo: better error handling. */
            }

            error = elf32_image_load( &image->subimages[i], img_loader, 0x00000000, ELF_LIBRARY );

            fd = img_loader->get_fd( img_loader->private );
            put_app_binary_loader( img_loader );
            sys_close( fd );

            if ( error != 0 ) {
                goto error2; /* todo: better error handling. */
            }
        }
    }

    return 0;

 error2:
    elf32_destroy_image_info( &image->info );

 error1:
    return error;
}

int elf32_context_init( elf32_context_t* context ) {
    memset( context, 0, sizeof( elf32_context_t ) );

    return 0;
}

static int elf32_context_get_symbol_helper( elf32_image_t* image, const char* name,
                                                         elf32_image_t** img, my_elf_symbol_t** sym ) {
    uint32_t i;
    my_elf_symbol_t* symbol;
    elf32_image_info_t* info;

    info = &image->info;

    for ( i = 0; i < info->needed_count; i++ ) {
        if ( elf32_context_get_symbol_helper( &image->subimages[i], name, img, sym ) == 0 ) {
            return 0;
        }
    }

    symbol = ( my_elf_symbol_t* )hashtable_get( &info->symbol_hash_table, ( const void* )name );

    if ( ( symbol != NULL ) &&
         ( symbol->section != 0 /* not undefined */ ) ) {
        *img = image;
        *sym = symbol;
        return 0;
    }


    return -ENOENT;
}

int elf32_context_get_symbol( elf32_context_t* context, const char* name,
                              elf32_image_t** image, my_elf_symbol_t** symbol ) {
    return elf32_context_get_symbol_helper( &context->main, name, image, symbol );
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
