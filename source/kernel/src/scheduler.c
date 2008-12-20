/* Scheduler
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

#include <thread.h>
#include <scheduler.h>
#include <console.h>

thread_t* first_ready = NULL;
thread_t* current = NULL;

int thread_test( void* arg ) {
    while ( true ) {
        kprintf( "%d", ( int )arg );
    }

    return 0;
}

thread_t* do_schedule( void ) {
    thread_t* next;

    if ( current != NULL ) {
        thread_t* tmp = first_ready;

        while ( tmp->queue_next != NULL ) {
            tmp = tmp->queue_next;
        }

        tmp->queue_next = current;
        current->queue_next = NULL;
    }

    next = first_ready;
    first_ready = first_ready->queue_next;

    return next;
}

int init_scheduler( void ) {
    thread_id id;
    thread_t* thread;

    id = create_kernel_thread( "thread1", thread_test, ( void* )1 );
    thread = get_thread_by_id( id );

    first_ready = thread;

    id = create_kernel_thread( "thread2", thread_test, ( void* )2 );
    thread = get_thread_by_id( id );

    first_ready->queue_next = thread;
    thread->queue_next = NULL;

    return 0;
}
