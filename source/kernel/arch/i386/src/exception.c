/* CPU exception handling functions
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
#include <scheduler.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/thread.h>
#include <arch/fpu.h>

void dump_registers( registers_t* regs ) {
    kprintf( "Error code: %d\n", regs->error_code );
    kprintf( "EAX=%x EBX=%x ECX=%x EDX=%x\n", regs->eax, regs->ebx, regs->ecx, regs->edx );
    kprintf( "ESI=%x EDI=%x\n", regs->esi, regs->edi );
    kprintf( "EBP=%x\n", regs->ebp );
    kprintf( "CS:EIP=%x:%x\n", regs->cs, regs->eip );

    if ( regs->cs & 3 ) {
        kprintf( "SS:ESP=%x:%x\n", regs->ss, regs->esp );
    }
}

void handle_division_by_zero( registers_t* regs ) {
    kprintf( "Division by zero!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_invalid_opcode( registers_t* regs ) {
    kprintf( "Invalid opcode!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_device_not_available( registers_t* regs ) {
    thread_t* thread;
    i386_thread_t* arch_thread;
    fpu_state_t* fpu_state;

    thread = current_thread();
    arch_thread = ( i386_thread_t* )thread->arch_data;
    fpu_state = arch_thread->fpu_state;

    spinlock_disable( &scheduler_lock );

    clear_task_switched();

    if ( ( arch_thread->flags & THREAD_FPU_USED ) == 0 ) {
        memset( fpu_state, 0, sizeof( fpu_state_t ) );

        fpu_state->fsave_data.control = 0x037F;
        fpu_state->fsave_data.tag = 0xFFFF;

        arch_thread->flags |= THREAD_FPU_USED;
    }

    load_fpu_state( fpu_state );
    arch_thread->flags |= THREAD_FPU_DIRTY;

    spinunlock_enable( &scheduler_lock );
}

void handle_general_protection_fault( registers_t* regs ) {
    kprintf( "General protection fault!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_fpu_exception( registers_t* regs ) {
    kprintf( "FPU exception!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_sse_exception( registers_t* regs ) {
    kprintf( "SSE exception!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}
