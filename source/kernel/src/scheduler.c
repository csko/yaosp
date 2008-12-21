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
#include <smp.h>
#include <kernel.h>

thread_t* first_ready;
thread_t* last_ready;

spinlock_t scheduler_lock = INIT_SPINLOCK;

int add_thread_to_ready( thread_t* thread ) {
    thread->state = THREAD_READY;
    thread->queue_next = NULL;

    if ( first_ready == NULL ) {
        first_ready = thread;
        last_ready = thread;
    } else {
        last_ready->queue_next = thread;
        last_ready = thread;
    }

    return 0;
}

thread_t* do_schedule( void ) {
    thread_t* next;
    thread_t* current;

    current = current_thread();

    if ( ( current != NULL ) && ( current != idle_thread() ) ) {
        switch ( current->state ) {
            case THREAD_RUNNING :
                add_thread_to_ready( current );
                break;

            case THREAD_WAITING :
            case THREAD_SLEEPING :
            case THREAD_ZOMBIE :
                break;

            default :
                panic(
                    "Thread %s with invalid state (%d) in the scheduler!\n",
                    current->name,
                    current->state
                );
                break;
        }
    }

    next = first_ready;

    if ( first_ready != NULL ) {
        first_ready = first_ready->queue_next;
    }

    if ( next == NULL ) {
        next = idle_thread();
    }

    return next;
}

int init_scheduler( void ) {
    first_ready = NULL;
    last_ready = NULL;

    return 0;
}
