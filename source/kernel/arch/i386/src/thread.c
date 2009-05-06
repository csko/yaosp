/* Architecture specific thread functions
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
#include <thread.h>
#include <config.h>
#include <errno.h>
#include <macros.h>
#include <syscall_table.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/thread.h>
#include <arch/gdt.h>
#include <arch/mm/config.h>

int arch_allocate_thread( thread_t* thread ) {
    uint32_t tmp;
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )kmalloc( sizeof( i386_thread_t ) + sizeof( fpu_state_t ) + 15 );

    if ( __unlikely( arch_thread == NULL ) ) {
        return -ENOMEM;
    }

    /* Calculate the address for the fpu state structure. Note
       that it has to be aligned to a 16 byte boundary! */

    tmp = ( uint32_t )( arch_thread + 1 );
    tmp = ( tmp + 15 ) & ~15;

    arch_thread->esp = 0;
    arch_thread->cr2 = 0;
    arch_thread->flags = 0;
    arch_thread->fpu_state = ( fpu_state_t* )tmp;
    arch_thread->signal_stack = NULL;

    thread->arch_data = ( void* )arch_thread;

    return 0;
}

void arch_destroy_thread( thread_t* thread ) {
    kfree( thread->arch_data );
}

int arch_create_kernel_thread( thread_t* thread, void* entry, void* arg ) {
    register_t* stack;
    registers_t* regs;
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )thread->arch_data;

    stack = ( register_t* )thread->kernel_stack_end;
    regs = ( registers_t* )( ( uint8_t* )stack - sizeof( registers_t ) );

    memset( regs, 0, sizeof( registers_t ) );

    regs->fs = KERNEL_DS;
    regs->es = KERNEL_DS;
    regs->ds = KERNEL_DS;
    regs->cs = KERNEL_CS;
    regs->eip = ( register_t )entry;
    regs->eflags = 0x203246;

    /* We don't have to specify ESP and SS here because returning
       to a kernel thread won't result in a privilege level change
       so those values won't be popped from the stack! */

    *--stack = ( register_t )arg;
    *--stack = ( register_t )kernel_thread_exit;

    arch_thread->esp = ( register_t )regs;

    return 0;
}

int arch_create_user_thread( thread_t* thread, void* entry, void* arg ) {
    register_t* stack;
    registers_t* regs;
    i386_thread_t* arch_thread;
    uint8_t exit_code[ 9 ];
    register_t exit_address;

    arch_thread = ( i386_thread_t* )thread->arch_data;

    stack = ( register_t* )thread->user_stack_end;
    regs = ( registers_t* )( ( uint8_t* )stack - ( sizeof( registers_t ) + 8 /* arg + ret. addr */ + 12 /* exit code */ ) );

    /* Create the exit code */

    /* movl %%eax, %%ebx
       movl $SYS_exit_thread, %eax
       int $0x80 */

    exit_code[ 0 ] = 0x89;
    exit_code[ 1 ] = 0xC3;
    exit_code[ 2 ] = 0xB8;
    exit_code[ 7 ] = 0xCD;
    exit_code[ 8 ] = 0x80;

    *( ( register_t* )&exit_code[ 3 ] ) = SYS_exit_thread;

    /* Put the exit code to the stack */

    stack -= 3;
    exit_address = ( register_t )stack;
    memcpy( ( void* )stack, exit_code, sizeof( exit_code ) );

    /* Initialize the top of the user stack */

    *--stack = ( register_t )arg;
    *--stack = exit_address;

    /* Prepare the registers on the top of the stack */

    memset( regs, 0, sizeof( registers_t ) );

    regs->fs = USER_DS | 3;
    regs->es = USER_DS | 3;
    regs->ds = USER_DS | 3;
    regs->cs = USER_CS | 3;
    regs->eip = ( register_t )entry;
    regs->eflags = 0x203246;
    regs->esp = ( register_t )( regs + 1 );
    regs->ss = USER_DS | 3;

    arch_thread->esp = ( register_t )regs;

    return 0;
}
