/* Architecture specific part of the kernel debugger
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
#include <debug.h>
#include <thread.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/io.h>
#include <arch/thread.h>
#include <arch/mm/config.h>

static uint8_t dbg_keymap[ 128 ] = {
    /* 0-9 */ 0, 0, '1', '2', '3', '4', '5', '6', '7', '8',
    /* 10-19 */ '9', '0', '-', '=', '\b', '\t', 'q', 'w', 'e', 'r',
    /* 20-29 */ 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    /* 30-39 */ 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
    /* 40-49 */ '\'', '`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n',
    /* 50-59 */ 'm', ',', '.', '/', 0, '*', 0, ' ', 0, 0,
    /* 60-69 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 70-79 */ 0, 0, 0, 0, '-', 0, 0, 0, '+', 0,
    /* 80-89 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 90-99 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 100-109 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 110-119 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /* 120-127 */ 0, 0, 0, 0, 0, 0, 0, 0
};

char arch_dbg_get_character( void ) {
    uint8_t code;
    uint8_t scancode;

    do {
        code = inb( 0x64 );

        if ( ( code & 1 ) &&
             ( code & ( 1 << 5 ) ) ) {
            inb( 0x60 );

            continue;
        }
    } while ( ( code & 1 ) == 0 );

    scancode = inb( 0x60 );

    if ( scancode & 0x80 ) {
        return 0;
    }

    scancode &= 0x7F;

    return dbg_keymap[ scancode ];
}

int debug_print_stack_trace( void ) {
    int error;
    register_t eip;
    register_t ebp;
    symbol_info_t symbol_info;

    ebp = get_ebp();

    kprintf( "Stack trace:\n" );

    while ( ebp != 0 ) {
        eip = *( ( register_t* )( ebp + 4 ) );

        kprintf( "  %x", eip );

        if ( eip >= FIRST_USER_ADDRESS ) {
            error = get_symbol_info( eip, &symbol_info );

            if ( error >= 0 ) {
                kprintf( " (%s+0x%x)", symbol_info.name, eip - symbol_info.address );
            }
        }

        kprintf( "\n" );

        ebp = *( ( register_t* )ebp );
    }

    return 0;
}

int arch_dbg_show_thread_info( struct thread* thread ) {
    registers_t* registers;
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )thread->arch_data;
    registers = ( registers_t* )arch_thread->esp;

    dbg_printf( "  kernel ESP: %08x\n", arch_thread->esp );
    dbg_printf( "  EIP: %08x\n", registers->eip );

    return 0;
}

int arch_dbg_trace_thread( struct thread* thread ) {
    int error;
    uint32_t ebp;
    uint32_t eip;
    ptr_t physical;
    registers_t* registers;
    i386_thread_t* arch_thread;

    arch_thread = ( i386_thread_t* )thread->arch_data;
    registers = ( registers_t* )arch_thread->esp;

    dbg_printf( "Tracing of thread %d:\n", thread->id );
    dbg_printf( "  0x%08x\n", registers->eip );

    ebp = registers->ebp;

    while ( ebp != 0 ) {
        error = memory_context_translate_address(
            thread->process->memory_context,
            ebp + 4,
            &physical
        );

        if ( error < 0 ) {
            dbg_printf( "Invalid address: %x\n", ebp + 4 );

            return 0;
        }

        eip = *( ( register_t* )physical );

        dbg_printf( "  0x%08x\n", eip );

        error = memory_context_translate_address(
            thread->process->memory_context,
            ebp,
            &physical
        );

        if ( error < 0 ) {
            dbg_printf( "Invalid address: %x\n", ebp );

            return 0;
        }

        ebp = *( ( register_t* )physical );
    }

    return 0;
}

void handle_debug_exception( registers_t* regs ) {
    start_kernel_debugger();
    halt_loop();
}
