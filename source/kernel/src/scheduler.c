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
#include <time.h>

thread_t* first_ready;
thread_t* last_ready;

thread_t* first_expired;
thread_t* last_expired;

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

int add_thread_to_expired( thread_t* thread ) {
    thread->state = THREAD_READY;
    thread->queue_next = NULL;

    reset_thread_quantum( thread );

    if ( first_expired == NULL ) {
        first_expired = thread;
        last_expired = thread;
    } else {
        last_expired->queue_next = thread;
        last_expired = thread;
    }

    return 0;
}

void reset_thread_quantum( thread_t* thread ) {
    thread->quantum = 50 * 1000;
}

static void swap_expired_and_ready_lists( void ) {
    first_ready = first_expired;
    last_ready = last_expired;

    first_expired = NULL;
    last_expired = NULL;
}

thread_t* do_schedule( void ) {
    uint64_t now;
    thread_t* next;
    thread_t* current;

    now = get_system_time();
    current = current_thread();

    if ( current != NULL ) {
        /* Calculate the time how long the previous thread
           was running */

        uint64_t runtime = now - current->exec_time;

        current->cpu_time += runtime;

        if ( current != idle_thread() ) {
            switch ( current->state ) {
                case THREAD_RUNNING :
                    if ( runtime >= current->quantum ) {
                        add_thread_to_expired( current );
                    } else {
                        current->quantum -= runtime;
                        add_thread_to_ready( current );
                    }

                    break;

                case THREAD_WAITING :
                case THREAD_SLEEPING :
                    current->quantum -= runtime;
                    /* TODO: this requires a bit more thinking during the
                       implementation of sleeping/waiting */
                    break;

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
    }

    /* Swap the expired and ready thread lists if the ready list is
       empty (this means that all runnable thread used its quantum) */

    if ( first_ready == NULL ) {
        swap_expired_and_ready_lists();
    }

    /* Get the first thread from the ready list */

    next = first_ready;

    if ( first_ready != NULL ) {
        first_ready = first_ready->queue_next;
    }

    /* If the ready list is empty then execute the idle thread */

    if ( next == NULL ) {
        next = idle_thread();
    }

    /* Save the execution time of the next thread */

    next->exec_time = now;

    return next;
}

int init_scheduler( void ) {
    first_ready = NULL;
    last_ready = NULL;

    first_expired = NULL;
    last_expired = NULL;

    return 0;
}
