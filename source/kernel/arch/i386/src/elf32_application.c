/* 32bit ELF application loader
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <loader.h>
#include <errno.h>
#include <console.h>
#include <smp.h>
#include <config.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/elf32.h>
#include <arch/gdt.h>
#include <arch/mm/paging.h>

static bool elf32_application_check( int fd ) {
    elf_header_t header;

    if ( sys_pread( fd, &header, sizeof( elf_header_t ), 0 ) != sizeof( elf_header_t ) ) {
        return false;
    }

    return elf32_check( &header );
}

static int elf32_parse_dynsym_section(
    int fd,
    elf_application_t* elf_application,
    elf_section_header_t* dynsym_section
) {
    uint32_t i;
    elf_symbol_t* symbols;
    elf_section_header_t* string_section;

    string_section = &elf_application->sections[ dynsym_section->link ];

    /* Load the string section */

    elf_application->strings = ( char* )kmalloc( string_section->size );

    if ( elf_application->strings == NULL ) {
        return -ENOMEM;
    }

    if ( sys_pread(
        fd,
        ( void* )elf_application->strings,
        string_section->size,
        string_section->offset
    ) != string_section->size ) {
        kfree( elf_application->strings );
        elf_application->strings = NULL;
        return -EIO;
    }

    /* Load symbols */

    symbols = ( elf_symbol_t* )kmalloc( dynsym_section->size );

    if ( symbols == NULL ) {
        kfree( elf_application->strings );
        elf_application->strings = NULL;
        return -ENOMEM;
    }

    if ( sys_pread(
        fd,
        ( void* )symbols,
        dynsym_section->size,
        dynsym_section->offset
    ) != dynsym_section->size ) {
        kfree( symbols );
        kfree( elf_application->strings );
        elf_application->strings = NULL;
        return -EIO;
    }

    /* Build our own symbol list */

    elf_application->symbol_count = dynsym_section->size / dynsym_section->entsize;

    elf_application->symbols = ( my_elf_symbol_t* )kmalloc(
        sizeof( my_elf_symbol_t ) * elf_application->symbol_count
    );

    if ( elf_application->symbols == NULL ) {
        kfree( elf_application->strings );
        elf_application->strings = NULL;
        kfree( symbols );
        return -ENOMEM;
    }

    for ( i = 0; i < elf_application->symbol_count; i++ ) {
        my_elf_symbol_t* my_symbol;
        elf_symbol_t* elf_symbol;

        my_symbol = &elf_application->symbols[ i ];
        elf_symbol = &symbols[ i ];

        my_symbol->name = elf_application->strings + elf_symbol->name;
        my_symbol->address = elf_symbol->value;
        my_symbol->info = elf_symbol->info;
    }

    kfree( symbols );

    return 0;
}

static int elf32_parse_dynamic_section(
    int fd,
    elf_application_t* elf_application,
    elf_section_header_t* dynamic_section
) {
    uint32_t i;
    uint32_t dyn_count;
    elf_dynamic_t* dyns;

    uint32_t rel_address = 0;
    uint32_t rel_size = 0;
    uint32_t pltrel_address = 0;
    uint32_t pltrel_size = 0;

    dyns = ( elf_dynamic_t* )kmalloc( dynamic_section->size );

    if ( dyns == NULL ) {
        return -ENOMEM;
    }

    if ( pread(
        fd,
        dyns,
        dynamic_section->size,
        dynamic_section->offset
    ) != dynamic_section->size ) {
        kfree( dyns );
        return -EIO;
    }

    dyn_count = dynamic_section->size / dynamic_section->entsize;

    for ( i = 0; i < dyn_count; i++ ) {
        switch ( dyns[ i ].tag ) {
            case DYN_REL :
                rel_address = dyns[ i ].value;
                break;

            case DYN_RELSZ :
                rel_size = dyns[ i ].value;
                break;

            case DYN_JMPREL :
                pltrel_address = dyns[ i ].value;
                break;

            case DYN_PLTRELSZ :
                pltrel_size = dyns[ i ].value;
                break;
        }
    }

    kfree( dyns );

    if ( ( rel_size > 0 ) || ( pltrel_size > 0 ) ) {
        uint32_t reloc_size = rel_size + pltrel_size;

        elf_application->reloc_count = reloc_size / sizeof( elf_reloc_t );
        elf_application->relocs = ( elf_reloc_t* )kmalloc( reloc_size );

        if ( elf_application->relocs == NULL ) {
            return -ENOMEM;
        }

        if ( rel_size > 0 ) {
            if ( sys_pread(
                fd,
                ( void* )elf_application->relocs,
                rel_size,
                rel_address
            ) != rel_size ) {
                kfree( elf_application->relocs );
                elf_application->relocs = NULL;
                return -EIO;
            }
        }

        if ( pltrel_size > 0 ) {
            if ( sys_pread(
                fd,
                ( char* )elf_application->relocs + rel_size,
                pltrel_size,
                pltrel_address
            ) != pltrel_size ) {
                kfree( elf_application->relocs );
                elf_application->relocs = NULL;
                return -EIO;
            }
        }
    }

    return 0;
}

static int elf32_parse_section_headers( int fd, elf_application_t* elf_application ) {
    int error;
    uint32_t i;
    elf_section_header_t* dynsym_section;
    elf_section_header_t* dynamic_section;
    elf_section_header_t* section_header;

    dynsym_section = NULL;
    dynamic_section = NULL;

    /* Loop through the section headers and save those ones we're
       interested in */

    for ( i = 0; i < elf_application->section_count; i++ ) {
        section_header = &elf_application->sections[ i ];

        switch ( section_header->type ) {
            case SECTION_DYNSYM :
                dynsym_section = section_header;
                break;

            case SECTION_DYNAMIC :
                dynamic_section = section_header;
                break;
        }
    }

    /* Handle dynsym section */

    if ( dynsym_section != NULL ) {
        error = elf32_parse_dynsym_section( fd, elf_application, dynsym_section );

        if ( error < 0 ) {
            return error;
        }
    }

    /* Handle dynamic section */

    if ( dynamic_section != NULL ) {
        error = elf32_parse_dynamic_section( fd, elf_application, dynamic_section );

        if ( error < 0 ) {
            return error;
        }
    }

    return 0;
}

static int elf32_application_map( int fd, elf_application_t* elf_application ) {
    int error;
    uint32_t i;
    elf_section_header_t* section_header;

    bool text_found = false;
    uint32_t text_start = 0;
    uint32_t text_end = 0;
    uint32_t text_size;
    uint32_t text_offset = 0;
    void* text_address;

    bool data_found = false;
    uint32_t data_start = 0;
    uint32_t data_end = 0;
    uint32_t data_size;
    uint32_t data_offset = 0;
    void* data_address;

    uint32_t bss_end = 0;
    uint32_t data_size_with_bss;

    for ( i = 0; i < elf_application->section_count; i++ ) {
        section_header = &elf_application->sections[ i ];

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

    elf_application->text_region = create_region(
        "ro",
        PAGE_ALIGN( text_size ),
        REGION_READ,
        ALLOC_LAZY,
        &text_address
    );

    if ( elf_application->text_region < 0 ) {
        return elf_application->text_region;
    }

    error = map_region_to_file( elf_application->text_region, fd, text_offset, text_size );

    if ( error < 0 ) {
        return error;
    }

    if ( data_found > 0 ) {
        elf_application->data_region = create_region(
            "rw",
            PAGE_ALIGN( data_size_with_bss ),
            REGION_READ | REGION_WRITE,
            ALLOC_LAZY,
            &data_address
        );

        if ( elf_application->data_region < 0 ) {
            /* TODO: delete text region */
            return elf_application->data_region;
        }

        if ( ( data_end != 0 ) && ( data_size > 0 ) ) {
            error = map_region_to_file( elf_application->data_region, fd, data_offset, data_size );

            if ( error < 0 ) {
                return error;
            }
        }
    }

    return 0;
}

static int elf32_application_load( int fd ) {
    int error;
    elf_header_t header;
    elf_application_t* elf_application;

    if ( sys_pread( fd, &header, sizeof( elf_header_t ), 0 ) != sizeof( elf_header_t ) ) {
        return -EIO;
    }

    elf_application = ( elf_application_t* )kmalloc( sizeof( elf_application_t ) );

    if ( elf_application == NULL ) {
        return -ENOMEM;
    }

    memset( elf_application, 0, sizeof( elf_application_t ) );

    if ( header.shentsize != sizeof( elf_section_header_t ) ) {
        kprintf( "ELF32: Invalid section header size!\n" );
        kfree( elf_application );
        return -EINVAL;
    }

    /* Load section headers from the ELF file */

    elf_application->section_count = header.shnum;

    elf_application->sections = ( elf_section_header_t* )kmalloc(
        sizeof( elf_section_header_t ) * elf_application->section_count
    );

    if ( elf_application->sections == NULL ) {
        kfree( elf_application );
        return -ENOMEM;
    }

    if ( sys_pread(
        fd,
        ( void* )elf_application->sections,
        sizeof( elf_section_header_t ) * elf_application->section_count,
        header.shoff
    ) != sizeof( elf_section_header_t ) * elf_application->section_count ) {
        kfree( elf_application->sections );
        kfree( elf_application );
        return -EIO;
    }

    /* Parse section headers */

    error = elf32_parse_section_headers( fd, elf_application );

    if ( error < 0 ) {
        kfree( elf_application->sections );
        kfree( elf_application );
        return error;
    }

    /* Map the ELF image to userspace */

    error = elf32_application_map( fd, elf_application );

    if ( error < 0 ) {
        /* TODO: free other stuffs */
        kfree( elf_application->sections );
        kfree( elf_application );
        return error;
    }

    elf_application->entry_address = header.entry;

    current_process()->loader_data = ( void* )elf_application;

    return 0;
}

int elf32_application_execute( void ) {
    thread_t* thread;
    registers_t* regs;
    elf_application_t* elf_application;

    /* Change the registers on the stack pushed by the syscall
       entry to return to the userspace */

    thread = current_thread();
    elf_application = ( elf_application_t* )thread->process->loader_data;

    regs = ( registers_t* )( thread->syscall_stack );

    regs->eax = 0;
    regs->ebx = 0;
    regs->ecx = 0;
    regs->edx = 0;
    regs->esi = 0;
    regs->edi = 0;
    regs->ebp = 0;
    regs->cs = USER_CS | 3;
    regs->ds = USER_DS | 3;
    regs->es = USER_DS | 3;
    regs->fs = USER_DS | 3;
    regs->eip = elf_application->entry_address;
    regs->esp = ( register_t )thread->user_stack_end;
    regs->ss = USER_DS | 3;

    return 0;
}

static application_loader_t elf32_application_loader = {
    .name = "ELF32",
    .check = elf32_application_check,
    .load = elf32_application_load,
    .execute = elf32_application_execute
};

__init int init_elf32_application_loader( void ) {
    register_application_loader( &elf32_application_loader );

    return 0;
}
