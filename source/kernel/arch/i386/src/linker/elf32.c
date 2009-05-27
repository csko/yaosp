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
#include <lib/string.h>

#include <arch/elf32.h>

extern multiboot_header_t mb_header;

bool elf32_check( elf_header_t* header ) {
    if ( ( header->ident[ ID_MAGIC0 ] != ELF32_MAGIC0 ) ||
         ( header->ident[ ID_MAGIC1 ] != ELF32_MAGIC1 ) ||
         ( header->ident[ ID_MAGIC2 ] != ELF32_MAGIC2 ) ||
         ( header->ident[ ID_MAGIC3 ] != ELF32_MAGIC3 ) ) {
        return false;
    }

    if ( header->ident[ ID_CLASS ] != ELF_CLASS_32 ) {
        return false;
    }

    if ( header->ident[ ID_DATA ] != ELF_DATA_2_LSB ) {
        return false;
    }

    if ( header->machine != ELF_EM_386 ) {
        return false;
    }

    return true;
}

#if 0
int elf32_load_and_validate_header( elf32_image_info_t* info, binary_loader_t* loader ) {
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

    if ( ( info->header.ident[ ID_MAGIC0 ] != ELF32_MAGIC0 ) ||
         ( info->header.ident[ ID_MAGIC1 ] != ELF32_MAGIC1 ) ||
         ( info->header.ident[ ID_MAGIC2 ] != ELF32_MAGIC2 ) ||
         ( info->header.ident[ ID_MAGIC3 ] != ELF32_MAGIC3 ) ||
         ( info->header.ident[ ID_CLASS ] != ELF_CLASS_32 ) ||
         ( info->header.ident[ ID_DATA ] != ELF_DATA_2_LSB ) ||
         ( info->header.machine != ELF_EM_386 ) ) {
        return -EINVAL;
    }

    return 0;
}

int elf32_load_section_headers( elf32_image_info_t* info, binary_loader_t* loader ) {
    int error;
    int sh_table_size;

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

    return 0;

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
    return -1;
}

int elf32_parse_section_headers( elf32_image_info_t* info, binary_loader_t* loader ) {
    int error;
    uint32_t i;
    elf_section_header_t* current;
    elf_section_header_t* strtab;
    elf_section_header_t* symtab;

    strtab = NULL;
    symtab = NULL;

    for ( i = 0, current = &info->section_headers[ i ]; i < info->header.shnum; i++, current++ ) {
        switch ( current->type ) {
            case SECTION_STRTAB :
                strtab = current;
                break;

            case SECTION_SYMTAB :
                symtab = current;
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

    return 0;
}

int elf32_init_image_info( elf32_image_info_t* info ) {
    info->section_headers = NULL;
    info->string_table = NULL;

    return 0;
}

int elf32_destroy_image_info( elf32_image_info_t* info ) {
    kfree( info->section_headers );
    info->section_headers = NULL;

    kfree( info->string_table );
    info->string_table = NULL;

    return 0;
}
#endif

__init int init_elf32_kernel_symbols( void ) {
    uint32_t i;
    elf_symbol_t* symbol;
    const char* string_table = NULL;
    const char* tmp_string_table;
    elf_section_header_t* strtab;
    elf_section_header_t* symtab;

    /* Do we have ELF information from the multiboot header? */

    if ( ( mb_header.flags & MB_FLAG_ELF_SYM_INFO ) == 0 ) {
        kprintf( "ELF symbol information not provided for the kernel!\n" );

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
        kprintf( "ELF symbol table not found for the kernel!\n" );

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
