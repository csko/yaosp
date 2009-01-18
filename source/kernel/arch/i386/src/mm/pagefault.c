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

#include <arch/cpu.h>
#include <arch/interrupt.h>

void dump_registers( registers_t* regs );

void handle_page_fault( registers_t* regs ) {
    thread_t* thread;

    kprintf( "Page fault!\n" );
    dump_registers( regs );
    kprintf( "CR2=%x\n", get_cr2() );
    kprintf( "CR3=%x\n", get_cr3() );

    thread = current_thread();

    if ( thread != NULL ) {
        kprintf( "Process: %s thread: %s\n", thread->process->name, thread->name );
    }

    disable_interrupts();
    halt_loop();
}
