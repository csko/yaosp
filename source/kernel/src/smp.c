/* Symmetric multi-processing
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

#include <smp.h>
#include <types.h>
#include <thread.h>
#include <errno.h>
#include <lib/string.h>

#include <arch/cpu.h>

cpu_t processor_table[ MAX_CPU_COUNT ];

process_t* current_process( void ) {
    return get_processor()->current_thread->process;
}

thread_t* current_thread( void ) {
    return get_processor()->current_thread;
}

thread_t* idle_thread( void ) {
    return get_processor()->idle_thread;
}

int init_smp( void ) {
    /* Make the boot CPU available */

    processor_table[ 0 ].present = true;
    processor_table[ 0 ].running = true;

    return 0;
}

int init_smp_late( void ) {
    int i;
    thread_id id;

    for ( i = 0; i < MAX_CPU_COUNT; i++ ) {
        if ( !processor_table[ i ].present ) {
            continue;
        }

        id = create_kernel_thread( "idle", ( thread_entry_t* )halt_loop, NULL );

        if ( id < 0 ) {
            return id;
        }

        processor_table[ i ].idle_thread = get_thread_by_id( id );

        if ( processor_table[ i ].idle_thread == NULL ) {
            return -EINVAL;
        }
    }

    return 0;
}
