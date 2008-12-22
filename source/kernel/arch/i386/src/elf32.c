/* 32bit ELF format handling
 *
 * Copyright (c) 2008 Zoltan Kovacs
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
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/elf32.h>

static bool elf32_module_check( void* data, size_t size ) {
    elf_header_t* header;

    if ( size < sizeof( elf_header_t ) ) {
        return false;
    }

    header = ( elf_header_t* )data;

    if ( ( header->ident[ ID_MAGIC0 ] != ELF32_MAGIC0 ) ||
         ( header->ident[ ID_MAGIC1 ] != ELF32_MAGIC1 ) ||
         ( header->ident[ ID_MAGIC2 ] != ELF32_MAGIC2 ) ||
         ( header->ident[ ID_MAGIC3 ] != ELF32_MAGIC3 ) ) {
        return false;
    }

    if ( header->ident[ ID_CLASS ] != ELF_CLASS_32 ) {
        return false;
    }

    /* TODO: check other stuffs, such as endian and machine */

    return true;
}

static int elf32_parse_section_headers( void* data, elf_module_t* elf_module ) {
    uint32_t i;
    elf_section_header_t* dynsym_section;
    elf_section_header_t* section_header;

    dynsym_section = NULL;

    /* Loop through the section headers and save those ones we're
       interested in */

    for ( i = 0; i < elf_module->section_count; i++ ) {
        section_header = &elf_module->sections[ i ];

        switch ( section_header->type ) {
            case SECTION_DYNSYM :
                dynsym_section = section_header;
                break;
        }
    }

    /* Handle dynsym section */

    if ( dynsym_section != NULL ) {
        elf_symbol_t* symbols;
        elf_section_header_t* string_section;

        string_section = &elf_module->sections[ dynsym_section->link ];

        /* Load the string section */

        elf_module->strings = ( char* )kmalloc( string_section->size );

        if ( elf_module->strings == NULL ) {
            return -ENOMEM;
        }

        memcpy(
            elf_module->strings,
            ( char* )data + string_section->offset,
            string_section->size
        );

        /* Load symbols */

        symbols = ( elf_symbol_t* )kmalloc( dynsym_section->size );

        if ( symbols == NULL ) {
            kfree( elf_module->strings );
            elf_module->strings = NULL;
            return -ENOMEM;
        }

        memcpy(
            symbols,
            ( char* )data + dynsym_section->offset,
            dynsym_section->size
        );

        /* Build our own symbol list */

        elf_module->symbol_count = dynsym_section->size / dynsym_section->entsize;

        elf_module->symbols = ( my_elf_symbol_t* )kmalloc(
            sizeof( my_elf_symbol_t ) * elf_module->symbol_count
        );

        if ( elf_module->symbols == NULL ) {
            kfree( elf_module->strings );
            kfree( symbols );
            elf_module->strings = NULL;
            return -ENOMEM;
        }

        kprintf( "Symbol count: %d\n", elf_module->symbol_count );

        for ( i = 0; i < elf_module->symbol_count; i++ ) {
            my_elf_symbol_t* my_symbol;
            elf_symbol_t* elf_symbol;

            my_symbol = &elf_module->symbols[ i ];
            elf_symbol = &symbols[ i ];

            my_symbol->name = elf_module->strings + elf_symbol->name;
            my_symbol->address = elf_symbol->value;

            kprintf( "Symbol %d: %s (%x)\n", i, my_symbol->name, my_symbol->address );
        }
    }

    return 0;
}

static module_t* elf32_module_load( void* data, size_t size ) {
    int error;
    module_t* module;
    elf_module_t* elf_module;
    elf_header_t* header;

    elf_module = ( elf_module_t* )kmalloc( sizeof( elf_module_t ) );

    if ( elf_module == NULL ) {
        return NULL;
    }

    memset( elf_module, 0, sizeof( elf_module_t ) );

    header = ( elf_header_t* )data;

    if ( header->shentsize != sizeof( elf_section_header_t ) ) {
        kprintf( "ELF32: Invalid section header size!\n" );
        kfree( elf_module );
        return NULL;
    }

    /* Load section headers from the ELF file */

    elf_module->section_count = header->shnum;

    elf_module->sections = ( elf_section_header_t* )kmalloc(
        sizeof( elf_section_header_t ) * elf_module->section_count
    );

    if ( elf_module->sections == NULL ) {
        kfree( elf_module );
        return NULL;
    }

    memcpy(
        elf_module->sections,
        ( char* )data + header->shoff,
        sizeof( elf_section_header_t ) * elf_module->section_count
    );

    /* Parse section headers */

    error = elf32_parse_section_headers( data, elf_module );

    if ( error < 0 ) {
        kfree( elf_module->sections );
        kfree( elf_module );
        return NULL;
    }

    module = create_module();

    if ( module == NULL ) {
        return NULL;
    }

    module->loader_data = ( void* )elf_module;

    return module;
}

static int elf32_module_free( module_t* module ) {
    /* TODO */

    return 0;
}

static bool elf32_module_get_symbol( module_t* module, const char* symbol_name, ptr_t* symbol_addr ) {
    uint32_t i;
    elf_module_t* elf_module;

    elf_module = ( elf_module_t* )module->loader_data;

    for ( i = 0; i < elf_module->symbol_count; i++ ) {
        if ( strcmp( elf_module->symbols[ i ].name, symbol_name ) == 0 ) {
            *symbol_addr = elf_module->symbols[ i ].address;
            return true;
        }
    }

    return false;
}

static module_loader_t elf32_module_loader = {
    .name = "ELF32",
    .check = elf32_module_check,
    .load = elf32_module_load,
    .free = elf32_module_free,
    .get_symbol = elf32_module_get_symbol
};

int init_elf32_loader( void ) {
    set_module_loader( &elf32_module_loader );

    return 0;
}
