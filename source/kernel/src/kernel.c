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
#include <module.h>
#include <semaphore.h>
#include <devices.h>
#include <loader.h>
#include <lib/stdarg.h>
#include <lib/string.h>

#include <arch/interrupt.h>
#include <arch/cpu.h>

int sys_dbprintf( const char* format, char** parameters ) {
    char buf[ 256 ];

    snprintf(
        buf,
        sizeof( buf ),
        format,
        parameters[ 0 ],
        parameters[ 1 ],
        parameters[ 2 ],
        parameters[ 3 ],
        parameters[ 4 ],
        parameters[ 5 ]
    );

    kprintf( "%s", buf );

    return 0;
}

void handle_panic( const char* file, int line, const char* format, ... ) {
    va_list args;
    kprintf( "Panic at %s:%d: ", file, line );

    va_start( args, format );
    kvprintf( format, args );
    va_end( args );

    disable_interrupts();
    halt_loop();
}

void kernel_main( void ) {
    int error;

    init_semaphores();
    init_devices();
    init_module_loader();
    init_application_loader();

    kprintf( "Initializing processes ... " );
    init_processes();
    kprintf( "done\n" );

    kprintf( "Initializing threads ... " );
    init_threads();
    kprintf( "done\n" );

    kprintf( "Initializing scheduler ... " );
    init_scheduler();
    kprintf( "done\n" );

    kprintf( "Initializing SMP ... " );
    init_smp();
    kprintf( "done\n" );

    error = arch_late_init();

    if ( error < 0 ) {
        return;
    }

    init_smp_late();

    /* Create the init thread */

    thread_id init_id = create_kernel_thread( "init", init_thread, NULL );

    if ( init_id < 0 ) {
        return;
    }

    wake_up_thread( init_id );

    /* Enable interrupts. The first timer interrupt will
       start the scheduler */

    enable_interrupts();

    halt_loop();
}
