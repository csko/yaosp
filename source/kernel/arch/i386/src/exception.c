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
#include <loader.h>
#include <sched/scheduler.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/thread.h>
#include <arch/fpu.h>

void dump_registers( registers_t* regs ) {
    kprintf( INFO, "Error code: %d\n", regs->error_code );
    kprintf( INFO, "EAX=%x EBX=%x ECX=%x EDX=%x\n", regs->eax, regs->ebx, regs->ecx, regs->edx );
    kprintf( INFO, "ESI=%x EDI=%x EBP=%x\n", regs->esi, regs->edi, regs->ebp );
    kprintf( INFO, "CS:EIP=%x:%x\n", regs->cs, regs->eip );

#if 0
    if ( error == 0 ) {
        kprintf( "CS:EIP=%x:%x (%s+0x%x)\n", regs->cs, regs->eip, symbol_info.name, regs->eip - symbol_info.address );
    } else {
        kprintf( "CS:EIP=%x:%x (no symbol)\n", regs->cs, regs->eip );
    }
#endif

    if ( regs->cs & 3 ) {
        kprintf( INFO, "SS:ESP=%x:%x\n", regs->ss, regs->esp );
    }
}

void handle_division_by_zero( registers_t* regs ) {
    kprintf( WARNING, "Division by zero!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_invalid_opcode( registers_t* regs ) {
    kprintf( WARNING, "Invalid opcode!\n" );
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

    scheduler_lock();

    clear_task_switched();

    if ( ( arch_thread->flags & THREAD_FPU_USED ) == 0 ) {
        memset( fpu_state, 0, sizeof( fpu_state_t ) );

        fpu_state->fsave_data.control = 0x037F;
        fpu_state->fsave_data.tag = 0xFFFF;

        arch_thread->flags |= THREAD_FPU_USED;
    }

    load_fpu_state( fpu_state );
    arch_thread->flags |= THREAD_FPU_DIRTY;

    scheduler_unlock();
}

void handle_general_protection_fault( registers_t* regs ) {
    kprintf( WARNING, "General protection fault!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_fpu_exception( registers_t* regs ) {
    kprintf( WARNING, "FPU exception!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}

void handle_sse_exception( registers_t* regs ) {
    kprintf( WARNING, "SSE exception!\n" );
    dump_registers( regs );
    disable_interrupts();
    halt_loop();
}
