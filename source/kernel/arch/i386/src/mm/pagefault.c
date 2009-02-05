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
#include <mm/region.h>
#include <mm/context.h>
#include <mm/pages.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/mm/paging.h>

extern semaphore_id region_lock;

void dump_registers( registers_t* regs );

static void invalid_page_fault( thread_t* thread, registers_t* regs, uint32_t cr2 ) {
    kprintf( "Invalid page fault at 0x%x\n", cr2 );
    dump_registers( regs );

    if ( thread != NULL ) {
        kprintf( "Process: %s thread: %s\n", thread->process->name, thread->name );
        thread_exit( 0 );
    } else {
        disable_interrupts();
        halt_loop();
    }
}

void handle_page_fault( registers_t* regs ) {
    uint32_t cr2;
    region_t* region;
    thread_t* thread;

    cr2 = get_cr2();

    thread = current_thread();

    if ( thread == NULL ) {
        invalid_page_fault( NULL, regs, cr2 );
    }

    LOCK( region_lock );

    region = do_memory_context_get_region_for( thread->process->memory_context, cr2 );

    /* TODO */
    /* NOTE: At the moment we don't expect page faults! */

    goto invalid;

    UNLOCK( region_lock );

    return;

invalid:
    UNLOCK( region_lock );

    invalid_page_fault( thread, regs, cr2 );
}
