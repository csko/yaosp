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
#include <macros.h>
#include <debug.h>
#include <signal.h>
#include <sched/scheduler.h>
#include <mm/region.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/mm/paging.h>

extern lock_id region_lock;

void dump_registers( registers_t* regs );

static void invalid_page_fault( thread_t* thread, registers_t* regs, uint32_t cr2, const char* message ) {
    kprintf( ERROR, "Invalid page fault at 0x%08x (%s)\n", cr2, message );
    dump_registers( regs );
    debug_print_stack_trace();

    if ( __likely( thread != NULL ) ) {
        kprintf( ERROR, "Process: %s thread: %s\n", thread->process->name, thread->name );
        memory_context_dump( thread->process->memory_context );

        if ( regs->error_code & 0x4 ) {
            send_signal( thread, SIGSEGV );
        } else {
            thread_exit( SIGSEGV );
        }
    } else {
        disable_interrupts();
        halt_loop();
    }
}

static int handle_cow_page( memory_context_t* context, uint32_t address ) {
    uint32_t ptr;
    uint32_t index;
    int copy_page;
    memory_page_t* page;

    i386_memory_context_t* arch_context;
    uint32_t* page_directory;
    uint32_t* page_table;

    arch_context = ( i386_memory_context_t* )context->arch_data;
    page_directory = arch_context->page_directory;

    index = PGD_INDEX( address );
    ASSERT( page_directory[ index ] != 0 );
    page_table = ( uint32_t* )( page_directory[ index ] & PAGE_MASK );

    index = PT_INDEX( address );
    ASSERT( page_table[ index ] != 0 );
    ptr = page_table[ index ] & PAGE_MASK;
    ASSERT( ( ptr != 0 ) && ( ptr < memory_size ) );

    spinlock_disable( &pages_lock );

    page = &memory_pages[ ptr / PAGE_SIZE ];
    ASSERT( page->ref_count > 0 );

    copy_page = ( page->ref_count > 1 );

    if ( copy_page ) {
        /* We have to copy this page */

        void* new_page = do_alloc_pages( &memory_descriptors[ MEM_COMMON ], 1 );

        if ( new_page == NULL ) {
            spinunlock_enable( &pages_lock );

            return -ENOMEM;
        }

        memcpy( new_page, ( void* )ptr, PAGE_SIZE );

        page->ref_count--;
        ASSERT( page->ref_count > 0 );

        page_table[ index ] = ( uint32_t )new_page | PAGE_PRESENT | PAGE_USER | PAGE_WRITE;
    } else {
        /* Copy-on-write already done on this page,
           simply give write access back. */

        page_table[ index ] |= PAGE_WRITE;
    }

    spinunlock_enable( &pages_lock );

    if ( copy_page ) {
        scheduler_lock();
        context->process->pmem_size += PAGE_SIZE;
        scheduler_unlock();
    }

    invlpg( address );

    return 0;
}

int handle_page_fault( registers_t* regs ) {
    int error;
    uint32_t cr2;
    thread_t* thread;
    memory_region_t* region;

    cr2 = get_cr2();
    thread = current_thread();

    region = memory_context_get_region_for( thread->process->memory_context, cr2 );

    /* If no region was found, this is an invalid memory access. */

    if ( region == NULL ) {
        invalid_page_fault( thread, regs, cr2, "invalid access" );
        return 0;
    }

    /* Handle copy-on-write pages */

    if ( ( regs->error_code == 7 ) &&
         ( region->flags & REGION_WRITE ) ) {
        error = handle_cow_page( thread->process->memory_context, cr2 );
    } else {
        error = -EINVAL;
    }

    memory_region_put( region );

    /* In case of any error, this is an invalid page fault */

    if ( error != 0 ) {
        invalid_page_fault( thread, regs, cr2, "???" );
    }

    return 0;
}
