/* Symmetric multi-processing
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

#include <smp.h>
#include <types.h>
#include <thread.h>
#include <errno.h>
#include <scheduler.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/atomic.h>
#include <arch/smp.h>

#ifdef ENABLE_SMP
int processor_count = 0;
#else
int processor_count = 1;
#endif /* ENABLE_SMP */

atomic_t active_processor_count = ATOMIC_INIT(0);
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

int get_active_processor_count( void ) {
    return atomic_get( &active_processor_count );
}

int init_smp( void ) {
    atomic_inc( &active_processor_count );

    /* Make the boot CPU available */

    processor_table[ 0 ].present = true;
    processor_table[ 0 ].running = true;

#ifdef ENABLE_SMP
    processor_activated();
#endif /* ENABLE_SMP */

    return 0;
}

int init_smp_late( void ) {
    int i;
    thread_id id;

#ifdef ENABLE_SMP
    /* If this is not an SMP system the processor_count won't be
       increased so we just fix the value here manually. */

    if ( processor_count == 0 ) {
        processor_count = 1;
    }
#endif

    /* Create the idle thread for present processors */

    for ( i = 0; i < MAX_CPU_COUNT; i++ ) {
        if ( !processor_table[ i ].present ) {
            continue;
        }

        id = create_kernel_thread(
            "idle",
            PRIORITY_IDLE,
            ( thread_entry_t* )halt_loop,
            NULL,
            PAGE_SIZE
        );

        if ( id < 0 ) {
            return id;
        }

        spinlock_disable( &scheduler_lock );

        processor_table[ i ].idle_thread = get_thread_by_id( id );

        spinunlock_enable( &scheduler_lock );

        if ( processor_table[ i ].idle_thread == NULL ) {
            return -EINVAL;
        }
    }

    return 0;
}
