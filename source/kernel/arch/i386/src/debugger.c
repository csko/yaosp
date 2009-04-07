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

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/mm/config.h>

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

void handle_debug_exception( registers_t* regs ) {
    kprintf( "Debug exception!\n" );
    disable_interrupts();
    halt_loop();
}
