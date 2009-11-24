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
#include <macros.h>
#include <linker/elf32.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/gdt.h>
#include <arch/mm/paging.h>

static bool elf32_application_check( binary_loader_t* loader ) {
    int error;
    elf32_image_info_t info;

    if ( elf32_init_image_info( &info ) != 0 ) {
        return false;
    }

    error = elf32_load_and_validate_header( &info, loader, ELF_TYPE_EXEC );

    elf32_destroy_image_info( &info );

    return ( error == 0 );
}

static int elf32_application_map( binary_loader_t* loader, elf_application_t* elf_application ) {
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

    for ( i = 0; i < elf_application->image_info.header.shnum; i++ ) {
        section_header = &elf_application->image_info.section_headers[ i ];

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

    elf_application->text_region = memory_region_create( "ro", PAGE_ALIGN( text_size ),
                                                         REGION_READ | REGION_EXECUTE );

    if ( elf_application->text_region == NULL ) {
        return -1;
    }

    memory_region_map_to_file(
        elf_application->text_region, loader->get_fd( loader->private ),
        text_offset, text_size
    );

    if ( data_found > 0 ) {
        elf_application->data_region = memory_region_create( "rw", PAGE_ALIGN( data_size_with_bss ),
                                                             REGION_READ | REGION_WRITE );

        if ( elf_application->data_region == NULL ) {
            /* TODO: delete text region */
            return -1;
        }

        if ( ( data_end != 0 ) && ( data_size > 0 ) ) {
            memory_region_map_to_file(
                elf_application->data_region, loader->get_fd( loader->private ),
                data_offset, data_size
            );
        } else {
            kprintf( WARNING, "%s(): nothing has been mapped to data region!\n", __FUNCTION__ );
        }
    }

    return 0;
}

static int elf32_application_load( binary_loader_t* loader ) {
    int error;
    elf_application_t* elf_application;

    elf_application = ( elf_application_t* )kmalloc( sizeof( elf_application_t ) );

    if ( elf_application == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    error = elf32_init_image_info( &elf_application->image_info );

    if ( __unlikely( error < 0 ) ) {
        goto error2;
    }

    error = elf32_load_and_validate_header( &elf_application->image_info, loader, ELF_TYPE_EXEC );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    error = elf32_load_section_headers( &elf_application->image_info, loader );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    error = elf32_parse_section_headers( &elf_application->image_info, loader );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    /* Map the ELF image to userspace */

    error = elf32_application_map( loader, elf_application );

    if ( __unlikely( error < 0 ) ) {
        goto error3;
    }

    current_process()->loader_data = ( void* )elf_application;

    return 0;

error3:
    elf32_destroy_image_info( &elf_application->image_info );

error2:
    kfree( elf_application );

error1:
    return error;
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
    regs->eip = elf_application->image_info.header.entry;
    regs->esp = ( register_t )thread->user_stack_end;
    regs->ss = USER_DS | 3;

    return 0;
}

static int elf32_application_get_symbol_info( thread_t* thread, ptr_t address, symbol_info_t* info ) {
    elf_application_t* elf_application;

    elf_application = ( elf_application_t* )thread->process->loader_data;

    return elf32_get_symbol_info( &elf_application->image_info, address, info );
}

static int elf32_application_destroy( void* data ) {
    elf_application_t* app;

    app = ( elf_application_t* )data;

    elf32_destroy_image_info( &app->image_info );
    kfree( app );

    return 0;
}

static application_loader_t elf32_application_loader = {
    .name = "ELF32",
    .check = elf32_application_check,
    .load = elf32_application_load,
    .execute = elf32_application_execute,
    .get_symbol_info = elf32_application_get_symbol_info,
    .destroy = elf32_application_destroy
};

__init int init_elf32_application_loader( void ) {
    register_application_loader( &elf32_application_loader );

    return 0;
}
