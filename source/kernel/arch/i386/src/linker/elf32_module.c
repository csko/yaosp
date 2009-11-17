/* 32bit ELF module loader
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

#include <module.h>
#include <console.h>
#include <errno.h>
#include <kernel.h>
#include <symbols.h>
#include <macros.h>
#include <linker/elf32.h>
#include <mm/kmalloc.h>
#include <mm/region.h>
#include <lib/string.h>

#include <arch/mm/config.h>

static bool elf32_module_check( binary_loader_t* loader ) {
    int error;
    elf32_image_info_t info;

    error = elf32_init_image_info( &info );

    if ( __unlikely( error < 0 ) ) {
        return false;
    }

    error = elf32_load_and_validate_header( &info, loader, ELF_TYPE_DYN );

    elf32_destroy_image_info( &info );

    return ( error == 0 );
}

static int elf32_module_map( elf_module_t* elf_module, binary_loader_t* loader ) {
    int error;
    uint32_t i;
    char region_name[ 64 ];
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
    uint32_t data_size_with_bss = 0;

    for ( i = 0; i < elf_module->image_info.header.shnum; i++ ) {
        section_header = &elf_module->image_info.section_headers[ i ];

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

    if ( ( !text_found ) || ( !data_found ) ) {
        error = -EINVAL;
        goto error1;
    }

    text_offset -= ( text_start & ~PAGE_MASK );
    text_start &= PAGE_MASK;
    text_size = text_end - text_start + 1;

    data_offset -= ( data_start & ~PAGE_MASK );
    data_start &= PAGE_MASK;
    data_size = data_end - data_start + 1;
    data_size_with_bss = bss_end - data_start + 1;

    snprintf( region_name, sizeof( region_name ), "%s_ro", loader->get_name( loader->private ) );

    elf_module->text_region = memory_region_create(
        region_name,
        PAGE_ALIGN( text_size ),
        REGION_READ | REGION_WRITE | REGION_KERNEL
    );

    if ( __unlikely( elf_module->text_region == NULL ) ) {
        error = -ENOMEM;
        goto error1;
    }

    if ( memory_region_alloc_pages( elf_module->text_region ) != 0 ) {
        error = -ENOMEM;
        goto error1; /* todo */
    }

    snprintf( region_name, sizeof( region_name ), "%s_rw", loader->get_name( loader->private ) );

    elf_module->data_region = memory_region_create(
        region_name,
        PAGE_ALIGN( data_size_with_bss ),
        REGION_READ | REGION_WRITE | REGION_KERNEL
    );

    if ( __unlikely( elf_module->data_region == NULL ) ) {
        error = -ENOMEM;
        goto error2;
    }

    if ( memory_region_alloc_pages( elf_module->data_region ) != 0 ) {
        error = -ENOMEM;
        goto error2; /* todo */
    }

    /* Copy text and data in */

    error = loader->read(
        loader->private,
        ( void* )elf_module->text_region->address,
        text_offset,
        text_size
    );

    if ( __unlikely( error != text_size ) ) {
        error = -EIO;
        goto error3;
    }

    error = loader->read(
        loader->private,
        ( void* )elf_module->data_region->address,
        data_offset,
        data_size
    );

    if ( __unlikely( error != data_size ) ) {
        error = -EIO;
        goto error3;
    }

    ASSERT( data_size_with_bss >= data_size );

    memset( ( char* )elf_module->data_region->address + data_size, 0, data_size_with_bss - data_size );

    elf_module->text_address = ( uint32_t )elf_module->text_region->address;
    elf_module->text_size = PAGE_ALIGN( text_size );

    return 0;

error3:
    memory_region_put( elf_module->data_region );
    elf_module->data_region = NULL;

error2:
    memory_region_put( elf_module->text_region );
    elf_module->text_region = NULL;

error1:
    return error;
}

static int elf32_relocate_module( elf_module_t* elf_module ) {
    int error;
    uint32_t i;
    uint32_t* target;
    elf_reloc_t* reloc;
    my_elf_symbol_t* symbol;
    my_elf_symbol_t* symbol_table;

    symbol_table = elf_module->image_info.dyn_symbol_table;

    for ( i = 0, reloc = &elf_module->image_info.reloc_table[ 0 ]; i < elf_module->image_info.reloc_count; i++, reloc++ ) {
        symbol = &symbol_table[ ELF32_R_SYM( reloc->info ) ];

        switch ( ELF32_R_TYPE( reloc->info ) ) {
            case R_386_32 : {
                my_elf_symbol_t* elf_symbol;

                elf_symbol = elf32_get_symbol( &elf_module->image_info, symbol->name );

                if ( elf_symbol == NULL ) {
                    return -EINVAL;
                }

                target = ( uint32_t* )( elf_module->text_address + reloc->offset );
                *target = *target + elf_symbol->address + elf_module->text_address;

                break;
            }

            case R_386_RELATIVE : {
                target = ( uint32_t* )( elf_module->text_address + reloc->offset );
                *target += elf_module->text_address;

                break;
            }

            case R_386_JMP_SLOT : {
                ptr_t address;

                error = get_kernel_symbol_address( symbol->name, &address );

                if ( error < 0 ) {
                    my_elf_symbol_t* elf_symbol;

                    elf_symbol = elf32_get_symbol( &elf_module->image_info, symbol->name );

                    if ( elf_symbol == NULL ) {
                        kprintf( ERROR, "ELF32(R_386_JMP_SLOT): Symbol %s not found!\n", symbol->name );

                        return -EINVAL;
                    }

                    address = elf_symbol->address + elf_module->text_address;
                }

                target = ( uint32_t* )( elf_module->text_address + reloc->offset );
                *target = ( uint32_t )address;

                break;
            }

            case R_386_GLOB_DATA : {
                my_elf_symbol_t* elf_symbol;

                elf_symbol = elf32_get_symbol( &elf_module->image_info, symbol->name );

                if ( elf_symbol == NULL ) {
                    kprintf( ERROR, "ELF32(R_386_GLOB_DATA): Symbol %s not found!\n", symbol->name );

                    return -EINVAL;
                }

                target = ( uint32_t* )( elf_module->text_address + reloc->offset );
                *target = elf_symbol->address + elf_module->text_address;

                break;
            }

            default :
                kprintf( WARNING, "elf32_reloacate_module(): Unknown reloc type: %x\n", ELF32_R_TYPE( reloc->info ) );

                return -EINVAL;
        }
    }

    return 0;
}

static int elf32_module_load( module_t* module, binary_loader_t* loader ) {
    int error;
    elf_module_t* elf_module;

    elf_module = ( elf_module_t* )kmalloc( sizeof( elf_module_t ) );

    if ( __unlikely( elf_module == NULL ) ) {
        error = -ENOMEM;
        goto error1;
    }

    error = elf32_init_image_info( &elf_module->image_info );

    if ( __unlikely( error < 0 ) ) {
        goto error2;
    }

    error = elf32_load_and_validate_header( &elf_module->image_info, loader, ELF_TYPE_DYN );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    error = elf32_load_section_headers( &elf_module->image_info, loader );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    error = elf32_parse_section_headers( &elf_module->image_info, loader );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    error = elf32_module_map( elf_module, loader );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    error = elf32_relocate_module( elf_module );

    if ( __unlikely( error < 0 ) ) {
        goto error4;
    }

    module->loader_data = ( void* )elf_module;

    return 0;

error4:
    /* TODO: unmap the module */

error3:
    elf32_destroy_image_info( &elf_module->image_info );

error2:
    kfree( elf_module );

error1:
    return error;
}

static bool elf32_module_get_symbol( module_t* module, const char* symbol_name, ptr_t* symbol_addr );

static int elf32_module_load_dependency_table( module_t* module, char* symbol, char*** dep_table, size_t* dep_count ) {
    bool found;
    char** module_dep_table;

    found = elf32_module_get_symbol( module, symbol, ( ptr_t* )&module_dep_table );

    if ( found ) {
        int count;

        for ( count = 0; module_dep_table[ count ] != NULL; count++ ) { }

        *dep_count = count;
        *dep_table = module_dep_table;
    } else {
        *dep_count = 0;
        *dep_table = NULL;
    }

    return 0;
}

int elf32_module_get_dependencies( module_t* module, module_dependencies_t* deps ) {
    int error;

    /* Load required dependency table */

    error = elf32_module_load_dependency_table(
        module,
        "__module_dependencies",
        &deps->dep_table,
        &deps->dep_count
    );

    if ( error < 0 ) {
        return error;
    }

    /* Load optional dependency table */

    error = elf32_module_load_dependency_table(
        module,
        "__module_optional_dependencies",
        &deps->optional_dep_table,
        &deps->optional_dep_count
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int elf32_module_free( module_t* module ) {
    /* TODO */

    return 0;
}

static bool elf32_module_get_symbol( module_t* module, const char* symbol_name, ptr_t* symbol_address ) {
    elf_module_t* elf_module;
    my_elf_symbol_t* elf_symbol;

    elf_module = ( elf_module_t* )module->loader_data;

    elf_symbol = elf32_get_symbol( &elf_module->image_info, symbol_name );

    if ( elf_symbol == NULL ) {
        return false;
    }

    *symbol_address = ( ptr_t )elf_module->text_address + elf_symbol->address;

    return true;
}

static int elf32_module_get_symbol_info( module_t* module, ptr_t address, symbol_info_t* info ) {
    int error;
    elf_module_t* elf_module;

    elf_module = ( elf_module_t* )module->loader_data;

    /* Module without symbols?! */

    if ( elf_module->image_info.symbol_count == 0 ) {
        return -EINVAL;
    }

    /* Make sure that the specified address is inside of this module */

    if ( ( address < elf_module->text_address ) ||
         ( address >= ( elf_module->text_address + elf_module->text_size ) ) ) {
        return -EINVAL;
    }

    error = elf32_get_symbol_info( &elf_module->image_info, address - elf_module->text_address, info );

    if ( error < 0 ) {
        return error;
    }

    info->address += elf_module->text_address;

    return 0;
}

static module_loader_t elf32_module_loader = {
    .name = "ELF32",
    .check_module = elf32_module_check,
    .get_dependencies = elf32_module_get_dependencies,
    .load_module = elf32_module_load,
    .free = elf32_module_free,
    .get_symbol = elf32_module_get_symbol,
    .get_symbol_info = elf32_module_get_symbol_info
};

__init int init_elf32_module_loader( void ) {
    set_module_loader( &elf32_module_loader );

    return 0;
}
