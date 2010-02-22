/* Scheduler
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
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
#include <console.h>
#include <smp.h>
#include <kernel.h>
#include <macros.h>
#include <debug.h>
#include <sched/scheduler.h>

waitqueue_t sleep_queue;
spinlock_t scheduler_lock = INIT_SPINLOCK( "scheduler" );

static thread_t* first_ready;
static thread_t* last_ready;

static thread_t* first_expired;
static thread_t* last_expired;

int add_thread_to_ready( thread_t* thread ) {
    ASSERT( scheduler_is_locked() );

    if ( thread->in_scheduler ) {
        return 0;
    }

    thread->state = THREAD_READY;
    thread->queue_next = NULL;
    thread->in_scheduler = 1;

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
    ASSERT( scheduler_is_locked() );

    if ( thread->in_scheduler ) {
        return 0;
    }

    thread->state = THREAD_READY;
    thread->queue_next = NULL;
    thread->in_scheduler = 1;

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

static void update_prev_thread( thread_t* thread, uint64_t now ) {
    cpu_t* cpu;
    uint64_t runtime;
    uint64_t time_used;

    /* Calculate the time how long the previous thread
       was running */

    runtime = now - thread->exec_time;
    time_used = now - thread->prev_checkpoint;

    thread->cpu_time += runtime;

    if ( thread->in_system ) {
        thread->sys_time += time_used;
    } else {
        thread->user_time += time_used;
    }

    /* Handle the idle thread separately */

    cpu = get_processor();

    if ( thread == cpu->idle_thread ) {
        thread->state = THREAD_WAITING;
        thread->in_scheduler = 1;

        cpu->idle_time += runtime;

        return;
    }

    switch ( thread->state ) {
        case THREAD_RUNNING :
            if ( runtime >= thread->quantum ) {
                add_thread_to_expired( thread );
            } else {
                thread->quantum -= runtime;
                add_thread_to_ready( thread );
            }

            break;

        case THREAD_READY :
            /* The thread tried to sleep but it was woken up before
               it could get to the scheduler */
            break;

        case THREAD_WAITING :
        case THREAD_SLEEPING :
            if ( runtime >= thread->quantum ) {
                /* 0 as a quantum is used to tell the wakeup functions
                   that this thread has to be added to the expired list
                   instead of the ready */

                thread->quantum = 0;
            } else {
                thread->quantum -= runtime;
            }

            break;

        case THREAD_ZOMBIE :
            break;

        default :
            panic(
                "Thread %s with invalid state (%d) in the scheduler!\n",
                thread->name,
                thread->state
            );

            break;
    }
}

static void update_next_thread( thread_t* thread, uint64_t now ) {
    thread->exec_time = now;
    thread->prev_checkpoint = now;
    thread->in_scheduler = 0;
}

thread_t* do_schedule( thread_t* current ) {
    uint64_t now;
    thread_t* next;

    now = get_system_time();

    if ( __likely( current != NULL ) ) {
        update_prev_thread( current, now );
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
        next->queue_next = NULL;
    }

    /* If the ready list is empty then execute the idle thread */

    if ( next == NULL ) {
        next = idle_thread();
    }

    /* Save the execution time of the next thread */

    update_next_thread( next, now );

    return next;
}

#ifdef ENABLE_DEBUGGER
int dbg_dump_ready_list( const char* params ) {
    thread_t* thread;

    thread = first_ready;

    if ( thread == NULL ) {
        dbg_printf( "There is no ready thread!\n" );
    } else {
        dbg_printf( "Ready threads:\n" );

        while ( thread != NULL ) {
            dbg_printf( "  %s\n", thread->name );
            thread = thread->queue_next;
        }
    }

    return 0;
}
#endif /* ENABLE_DEBUGGER */

__init int init_scheduler( void ) {
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
