/* i386 specific signal handling functions
 *
 * Copyright (c) 2009 Zoltan Kovacs
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
#include <smp.h>
#include <macros.h>

#include <arch/thread.h>
#include <arch/gdt.h>

#include <console.h>

int arch_handle_signals( registers_t* regs ) {
    thread_t* thread;
    i386_thread_t* arch_thread;

    /* Perform signal handling only if we return to userspace */

    if ( regs->cs != ( USER_CS | 3 ) ) {
        return 0;
    }

    thread = current_thread();

    /* Check if we have any pending signal */

    if ( !is_signal_pending( thread ) ) {
        return 0;
    }

    /* Save the stack pointer to the architecture specific part of the thread */

    arch_thread = ( i386_thread_t* )thread->arch_data;

    ASSERT( arch_thread->signal_stack == NULL );

    arch_thread->signal_stack = ( void* )regs;

    /* Call the architecture independent part of the signal handler code */

    handle_signals( thread );

    /* Reset the saved signal stack */

    arch_thread->signal_stack = NULL;

    return 0;
}
