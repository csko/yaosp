/* Architecture part of the scheduler
 *
 * Copyright (c) 2008 Zoltan Kovacs
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
#include <scheduler.h>

#include <arch/thread.h>

extern thread_t* current;

void switch_to_thread( register_t esp );

void schedule( registers_t* regs ) {
    thread_t* next;
    i386_thread_t* arch_thread;

    if ( current != NULL ) {
        arch_thread = ( i386_thread_t* )current->arch_data;

        arch_thread->esp = ( register_t )regs;
    }

    next = do_schedule();
    arch_thread = ( i386_thread_t* )next->arch_data;

    current = next;

    switch_to_thread( arch_thread->esp );
}
