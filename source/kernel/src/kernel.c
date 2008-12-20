/* Miscellaneous kernel functions
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

#include <console.h>
#include <process.h>
#include <thread.h>
#include <scheduler.h>
#include <kernel.h>

#include <arch/interrupt.h>
#include <arch/cpu.h>

void handle_panic( const char* file, int line, const char* format, ... ) {
    kprintf( "Panic at %s:%d!\n", file, line );
    disable_interrupts();
    halt_loop();
}

void kernel_main( void ) {
    int error;

    kprintf( "Initializing processes ... " );
    init_processes();
    kprintf( "done\n" );

    kprintf( "Initializing threads ... " );
    init_threads();
    kprintf( "done\n" );

    kprintf( "Initializing scheduler ... " );
    init_scheduler();
    kprintf( "done\n" );

    error = arch_late_init();

    if ( error < 0 ) {
        return;
    }

    enable_interrupts();

    halt_loop();
}
