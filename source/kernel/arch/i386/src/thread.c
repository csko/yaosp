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
    uint32_t tmp;
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )kmalloc( sizeof( i386_thread_t ) );

    if ( arch_thread == NULL ) {
        goto error1;
    }

    tmp = ( uint32_t )kmalloc( sizeof( fpu_state_t ) + 15 );

    if ( tmp == 0 ) {
        goto error2;
    }

    arch_thread->esp = 0;
    arch_thread->flags = 0;
    arch_thread->fpu_state_base = ( void* )tmp;
    arch_thread->fpu_state = ( fpu_state_t* )( ( tmp + 15 ) & ~15 );

    thread->arch_data = ( void* )arch_thread;

    return 0;

error2:
    kfree( arch_thread );

error1:
    return -ENOMEM;
}

void arch_destroy_thread( thread_t* thread ) {
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )thread->arch_data;

    kfree( arch_thread->fpu_state_base );
    kfree( thread->arch_data );
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
