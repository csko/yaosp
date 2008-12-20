/* Architecture specific thread functions
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
#include <thread.h>
#include <config.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/thread.h>
#include <arch/gdt.h>
#include <arch/mm/config.h>

int arch_allocate_thread( thread_t* thread ) {
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )kmalloc( sizeof( i386_thread_t ) );

    if ( arch_thread == NULL ) {
        return -ENOMEM;
    }

    memset( arch_thread, 0, sizeof( arch_thread ) );

    thread->arch_data = ( void* )arch_thread;

    return 0;
}

int arch_create_kernel_thread( thread_t* thread, void* entry, void* arg ) {
    register_t* stack;
    registers_t* regs;
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )thread->arch_data;

    stack = ( register_t* )( ( uint8_t* )thread->kernel_stack + KERNEL_STACK_PAGES * PAGE_SIZE );
    regs = ( registers_t* )( ( uint8_t* )stack - sizeof( registers_t ) );
    
    memset( regs, 0, sizeof( registers_t ) );

    regs->fs = KERNEL_DS;
    regs->es = KERNEL_DS;
    regs->ds = KERNEL_DS;
    regs->cs = KERNEL_CS;
    regs->eip = ( unsigned long )entry;
    regs->eflags = 0x203246;

    /* We don't have to specify ESP and SS here because returning
       to a kernel thread won't result in a privilege level change
       so those values won't be popped from the stack! */

    *--stack = ( register_t )arg;
    *--stack = ( register_t )kernel_thread_exit;

    arch_thread->esp = ( register_t )regs;

    return 0;
}
