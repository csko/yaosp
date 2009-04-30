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
#include <syscall_table.h>
#include <lib/string.h>

#include <arch/thread.h>
#include <arch/gdt.h>
#include <arch/signal.h>

#include <console.h>
#include <thread.h>

int sys_signal_return( void ) {
    uint32_t* stack;
    thread_t* thread;
    registers_t* regs;
    i386_thread_t* arch_thread;
    i386_sig_context_t* sig_context;

    thread = current_thread();
    arch_thread = ( i386_thread_t* )thread->arch_data;

    regs = ( registers_t* )thread->syscall_stack;
    stack = ( uint32_t* )regs->esp;

    sig_context = ( i386_sig_context_t* )stack;

    memcpy( regs, &sig_context->regs, sizeof( registers_t ) );
    arch_thread->cr2 = sig_context->cr2;

    return 0;
}

int arch_handle_userspace_signal( thread_t* thread, int signal, struct sigaction* handler ) {
    uint8_t* code;
    uint32_t* stack;
    registers_t* regs;
    i386_thread_t* arch_thread;
    i386_sig_context_t* sig_context;

    arch_thread = ( i386_thread_t* )thread->arch_data;

    ASSERT( arch_thread->signal_stack != NULL );

    regs = ( registers_t* )arch_thread->signal_stack;
    stack = ( uint32_t* )regs->esp;

    ASSERT( ( sizeof( i386_sig_context_t ) % 4 ) == 0 );

    /* Save sigcontext on the stack */

    stack -= ( sizeof( i386_sig_context_t ) / 4 );

    sig_context = ( i386_sig_context_t* )stack;

    memcpy( &sig_context->regs, regs, sizeof( registers_t ) );
    sig_context->cr2 = arch_thread->cr2;

    /* Put the return code to the stack */

    stack -= 4;

    code = ( uint8_t* )stack;

    /**
     * addl $stack_skip * 4, %esp
     * movl $SYS_signal_return, %eax
     * int $0x80
     */

    code[ 0 ] = 0x81;
    code[ 1 ] = 0xC4;
    code[ 6 ] = 0xB8;
    code[ 11 ] = 0xCD;
    code[ 12 ] = 0x80;

    *( ( uint32_t* )( code + 2 ) ) = ( 4 /* return code */ + 1 /* signal number */ ) * 4;
    *( ( uint32_t* )( code + 7 ) ) = SYS_signal_return;

    /* Put signal number and return address to the stack */

    stack -= 2;

    stack[ 0 ] = ( uint32_t )code;
    stack[ 1 ] = ( uint32_t )( signal * 4 );

    regs->eip = ( register_t )handler->sa_handler;
    regs->esp = ( register_t )stack;

    return 0;
}

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
