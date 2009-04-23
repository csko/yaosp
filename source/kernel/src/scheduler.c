/* Scheduler
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

#include <thread.h>
#include <scheduler.h>
#include <console.h>
#include <smp.h>
#include <kernel.h>
#include <macros.h>

#include <arch/pit.h> /* get_system_time() */

thread_t* first_ready;
thread_t* last_ready;

thread_t* first_expired;
thread_t* last_expired;

waitqueue_t sleep_queue;
spinlock_t scheduler_lock = INIT_SPINLOCK( "scheduler" );

int add_thread_to_ready( thread_t* thread ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );

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
    ASSERT( spinlock_is_locked( &scheduler_lock ) );

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

    if ( __likely( current != NULL ) ) {
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

                case THREAD_READY :
                    /* The thread tried to sleep but it was woken up before
                       it could get to the scheduler */
                    break;

                case THREAD_WAITING :
                case THREAD_SLEEPING :
                    if ( runtime >= current->quantum ) {
                        /* 0 as a quantum is used to tell the wakeup functions
                           that this thread has to be added to the expired list
                           instead of the ready */

                        current->quantum = 0;
                    } else {
                        current->quantum -= runtime;
                    }

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
        } else {
            current->state = THREAD_WAITING;
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

void lock_scheduler( void ) {
    spinlock_disable( &scheduler_lock );
}

void unlock_scheduler( void ) {
    spinunlock_enable( &scheduler_lock );
}

int __init init_scheduler( void ) {
    int error;

    first_ready = NULL;
    last_ready = NULL;

    first_expired = NULL;
    last_expired = NULL;

    /* Initialize the sleep queue */

    error = init_waitqueue( &sleep_queue );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
