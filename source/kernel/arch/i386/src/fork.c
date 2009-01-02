/* Fork implementation
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
#include <thread.h>
#include <smp.h>
#include <kernel.h>
#include <lib/string.h>

#include <arch/thread.h>

int arch_do_fork( thread_t* old_thread, thread_t* new_thread ) {
    registers_t* old_regs;
    registers_t* new_regs;
    i386_thread_t* new_arch_thread;

    old_regs = ( registers_t* )old_thread->syscall_stack;

    /* Clone the registers on the stack */

    if ( old_regs->cs & 3 ) {
        /* fork is called from userspace */

        register_t* stack = ( register_t* )( ( uint8_t* )new_thread->kernel_stack + KERNEL_STACK_PAGES * PAGE_SIZE );
        new_regs = ( registers_t* )( ( uint8_t* )stack - sizeof( registers_t ) );

        memcpy( new_regs, old_regs, sizeof( registers_t ) );
    } else {
        /* fork is called from kernel */

        register_t* stack = ( register_t* )( ( uint8_t* )new_thread->kernel_stack + KERNEL_STACK_PAGES * PAGE_SIZE );
        new_regs = ( registers_t* )( ( uint8_t* )stack - sizeof( registers_t ) + 2 * sizeof( register_t ) );

        memcpy( new_regs, old_regs, sizeof( registers_t ) - 2 * sizeof( register_t ) );
    }

    /* Make the return value 0 for the new thread */

    new_regs->eax = 0;

    /* Set the ESP value of the new thread */

    new_arch_thread = ( i386_thread_t* )new_thread->arch_data;
    new_arch_thread->esp = ( register_t )new_regs;

    return 0;
}
