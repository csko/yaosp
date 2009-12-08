/* Architecture specific part of the scheduler
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
#include <smp.h>
#include <macros.h>
#include <sched/scheduler.h>

#include <arch/thread.h>
#include <arch/cpu.h>
#include <arch/mm/paging.h>

void switch_to_thread( register_t esp );

int schedule( registers_t* regs ) {
    thread_t* next;
    thread_t* current;
    i386_cpu_t* arch_cpu;
    i386_thread_t* arch_thread;
    i386_memory_context_t* arch_mem_context;

    /* Lock the scheduler */

    scheduler_lock();

    /* Save the state of the previously running thread */

    current = current_thread();

    if ( __likely( current != NULL ) ) {
        arch_thread = ( i386_thread_t* )current->arch_data;

        /* Save the current ESP and CR2 value */

        arch_thread->esp = ( register_t )regs;
        arch_thread->cr2 = get_cr2();

        /* Save the FPU state if it is dirty */

        if ( arch_thread->flags & THREAD_FPU_DIRTY ) {
            save_fpu_state( arch_thread->fpu_state );
            arch_thread->flags &= ~THREAD_FPU_DIRTY;
        }
    }

    /* Ask the scheduler to select the next thread to run */

    next = do_schedule( current );

    arch_thread = ( i386_thread_t* )next->arch_data;
    arch_mem_context = ( i386_memory_context_t* )next->process->memory_context->arch_data;

    get_processor()->current_thread = next;
    next->state = THREAD_RUNNING;

    arch_cpu = ( i386_cpu_t* )get_processor()->arch_data;

    arch_cpu->tss.esp0 = ( register_t )( ( uint8_t* )next->kernel_stack_end - sizeof( register_t ) );

    /* Load the CR2 register of the new thread */

    set_cr2( arch_thread->cr2 );

    /* Set the new memory context if needed */

    if ( ( current == NULL ) || ( current->process != next->process ) ) {
        set_cr3( ( uint32_t )arch_mem_context->page_directory );
        arch_cpu->tss.cr3 = ( register_t )arch_mem_context->page_directory;
    }

    set_task_switched();

    /* Unlock the scheduler spinlock. The iret instruction in
       switch_to_thread() will enable interrupts if required. */

    spinunlock( &scheduler_lock );

    /* Switch to the next thread */

    switch_to_thread( arch_thread->esp );

    return 0;
}
