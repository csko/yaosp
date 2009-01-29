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
#include <arch/cpu.h>

int arch_do_fork( thread_t* old_thread, thread_t* new_thread ) {
    registers_t* old_regs;
    registers_t* new_regs;
    i386_thread_t* old_arch_thread;
    i386_thread_t* new_arch_thread;

    old_regs = ( registers_t* )old_thread->syscall_stack;

    old_arch_thread = ( i386_thread_t* )old_thread->arch_data;
    new_arch_thread = ( i386_thread_t* )new_thread->arch_data;

    /* Clone the FPU state */

    if ( old_arch_thread->flags & THREAD_FPU_USED ) {
        if ( old_arch_thread->flags & THREAD_FPU_DIRTY ) {
            save_fpu_state( old_arch_thread->fpu_state );
            old_arch_thread->flags &= ~THREAD_FPU_DIRTY; 
            set_task_switched();
        }

        memcpy( new_arch_thread->fpu_state, old_arch_thread->fpu_state, sizeof( fpu_state_t ) );
    }

    /* Clone the registers on the stack */

    new_regs = ( registers_t* )( ( uint8_t* )new_thread->kernel_stack_end - sizeof( registers_t ) );

    memcpy( new_regs, old_regs, sizeof( registers_t ) );

    /* Make the return value 0 for the new thread */

    new_regs->eax = 0;

    /* Set the ESP value of the new thread */

    new_arch_thread->esp = ( register_t )new_regs;

    return 0;
}
