/* Page fault handler
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
#include <smp.h>
#include <errno.h>
#include <semaphore.h>
#include <macros.h>
#include <scheduler.h>
#include <mm/region.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/mm/paging.h>

extern semaphore_id region_lock;

void dump_registers( registers_t* regs );

static void invalid_page_fault( thread_t* thread, registers_t* regs, uint32_t cr2, const char* message ) {
    kprintf( "Invalid page fault at 0x%x (%s)\n", cr2, message );
    dump_registers( regs );

    if ( thread != NULL ) {
        kprintf( "Process: %s thread: %s\n", thread->process->name, thread->name );
        memory_context_dump( thread->process->memory_context );
        thread_exit( 0 );
    } else {
        disable_interrupts();
        halt_loop();
    }
}

static int handle_lazy_page_allocation( region_t* region, uint32_t address ) {
    void* p;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;
    memory_context_t* context;
    i386_memory_context_t* arch_context;

    context = region->context;
    arch_context = ( i386_memory_context_t* )context->arch_data;

    pgd_entry = page_directory_entry( arch_context, address );

    if ( *pgd_entry == 0 ) {
        p = alloc_pages( 1, MEM_COMMON );

        if ( p == NULL ) {
            return -ENOMEM;
        }

        memsetl( p, 0, PAGE_SIZE / 4 );

        *pgd_entry = ( uint32_t )p | PRESENT | WRITE | USER;
    }

    pt_entry = page_table_entry( *pgd_entry, address );

    p = alloc_pages( 1, MEM_COMMON );

    if ( p == NULL ) {
        return -ENOMEM;
    }

    memsetl( p, 0, PAGE_SIZE / 4 );

    *pt_entry = ( uint32_t )p | PRESENT | WRITE | USER;

    /* Handle memory mapped files */

    if ( region->file != NULL ) {
        int data;
        size_t to_read;
        off_t file_read_offset;
        uint32_t region_offset;

        ASSERT( address >= region->start );

        region_offset = address - region->start;

        if ( ( region_offset & PAGE_MASK ) < region->file_size ) {
            file_read_offset = region->file_offset + ( region_offset & PAGE_MASK );
            to_read = MIN( PAGE_SIZE, ( region->file_offset + region->file_size - file_read_offset ) );

            data = do_pread_helper( region->file, p, to_read, file_read_offset );

            if ( data != to_read ) {
                return -EIO;
            }
        }
    }

    /* Update pmem of the current process */

    spinlock_disable( &scheduler_lock );

    context->process->pmem_size += PAGE_SIZE;

    spinunlock_enable( &scheduler_lock );

    return 0;
}

void handle_page_fault( registers_t* regs ) {
    int error;
    uint32_t cr2;
    region_t* region;
    thread_t* thread;
    const char* message;

    cr2 = get_cr2();

    thread = current_thread();

    if ( thread == NULL ) {
        invalid_page_fault( NULL, regs, cr2, "unknown" );
    }

    LOCK( region_lock );

    region = do_memory_context_get_region_for( thread->process->memory_context, cr2 );

    if ( region == NULL ) {
        message = "no region for address";
        goto invalid;
    }

    if ( region->alloc_method == ALLOC_LAZY ) {
        error = handle_lazy_page_allocation( region, cr2 );

        if ( error < 0 ) {
            message = "lazy page allocation failed";
            goto invalid;
        }
    } else {
        message = "unknown";
        goto invalid;
    }

    UNLOCK( region_lock );

    return;

invalid:
    UNLOCK( region_lock );

    invalid_page_fault( thread, regs, cr2, message );
}
