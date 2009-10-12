/* Condition variable implementation
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#include <errno.h>
#include <macros.h>
#include <smp.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <lock/condition.h>
#include <lock/mutex.h>
#include <lock/common.h>
#include <sched/scheduler.h>
#include <lib/string.h>

#include <arch/pit.h>

int do_acquire_mutex( lock_context_t* context, mutex_t* mutex, thread_t* thread,
                      time_t timeout, bool try_lock, int flags );
void do_release_mutex( mutex_t* mutex );

static int do_wait_condition( lock_context_t* context, lock_id condition_id, lock_id mutex_id, time_t timeout ) {
    int error;
    mutex_t* mutex;
    thread_t* thread;
    condition_t* condition;
    lock_header_t* header;
    uint64_t wakeup_time;

    wakeup_time = get_system_time() + timeout;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, condition_id );

    if ( __unlikely( ( header == NULL ) ||
                     ( header->type != CONDITION ) ) ) {
        spinunlock_enable( &context->lock );

        kprintf(
            WARNING,
            "do_wait_condition(): Invalid condition id or the specified lock is not a condition!\n"
        );

        return -EINVAL;
    }

    condition = ( condition_t* )header;

    header = lock_context_get( context, mutex_id );

    if ( __unlikely( ( header == NULL ) ||
                     ( header->type != MUTEX ) ) ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    mutex = ( mutex_t* )header;

    thread = current_thread();

    /* Make sure that the mutex is locked by the current thread */

    if ( mutex->holder != thread->id ) {
        spinunlock_enable( &context->lock );

        return -EPERM;
    }

    /* Release the mutex */

    do_release_mutex( mutex );

    /* Wait for the conditional variable */

    if ( timeout != INFINITE_TIMEOUT ) {
        error = lock_timed_wait_on( context, thread, CONDITION, condition_id, &condition->waiters, wakeup_time );
    } else {
        error = lock_wait_on( context, thread, CONDITION, condition_id, &condition->waiters );
    }

    /* Acquire the mutex */

    do_acquire_mutex( context, mutex, thread, INFINITE_TIMEOUT, false, LOCK_IGNORE_SIGNAL );

    spinunlock_enable( &context->lock );

    return error;
}

int condition_wait( lock_id condition, lock_id mutex ) {
    return do_wait_condition( &kernel_lock_context, condition, mutex, INFINITE_TIMEOUT );
}

int condition_timedwait( lock_id condition, lock_id mutex, time_t timeout ) {
    return do_wait_condition( &kernel_lock_context, condition, mutex, timeout );
}

static int do_signal_condition( lock_context_t* context, lock_id condition_id ) {
    lock_header_t* header;
    condition_t* condition;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, condition_id );

    if ( __unlikely( ( header == NULL ) ||
                     ( header->type != CONDITION ) ) ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    condition = ( condition_t* )header;

    spinlock( &scheduler_lock );
    waitqueue_wake_up_head( &condition->waiters, 1 );
    spinunlock( &scheduler_lock );

    spinunlock_enable( &context->lock );

    return 0;
}

int condition_signal( lock_id condition ) {
    return do_signal_condition( &kernel_lock_context, condition );
}

static int do_broadcast_condition( lock_context_t* context, lock_id condition_id ) {
    lock_header_t* header;
    condition_t* condition;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, condition_id );

    if ( __unlikely( ( header == NULL ) ||
                     ( header->type != CONDITION ) ) ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    condition = ( condition_t* )header;

    spinlock( &scheduler_lock );
    waitqueue_wake_up_all( &condition->waiters );
    spinunlock( &scheduler_lock );

    spinunlock_enable( &context->lock );

    return 0;
}

int condition_broadcast( lock_id condition ) {
    return do_broadcast_condition( &kernel_lock_context, condition );
}

int condition_clone( condition_t* old, condition_t* new ) {
    int error;

    error = init_waitqueue( &new->waiters );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int condition_update( condition_t* condition, thread_id new_thread ) {
    return 0;
}

static lock_id do_create_condition( lock_context_t* context, const char* name ) {
    int error;
    size_t name_length;
    lock_header_t* header;
    condition_t* condition;

    name_length = strlen( name );

    condition = ( condition_t* )kmalloc( sizeof( condition_t ) + name_length + 1 );

    if ( condition == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* Initialize the lock header */

    header = ( lock_header_t* )condition;

    header->type = CONDITION;
    header->name = ( char* )( condition + 1 );
    memcpy( header->name, name, name_length + 1 );

    /* Initialize condition specific fields */

    error = init_waitqueue( &condition->waiters );

    if ( error < 0 ) {
        goto error2;
    }

    /* Insert to the lock context */

    error = lock_context_insert( context, header );

    if ( error < 0 ) {
        goto error2;
    }

    return header->id;

 error2:
    kfree( condition );

 error1:
    return error;
}

lock_id condition_create( const char* name ) {
    return do_create_condition( &kernel_lock_context, name );
}

static int do_destroy_condition( lock_context_t* context, lock_id condition_id ) {
    int error;
    lock_header_t* header;
    condition_t* condition;

    error = lock_context_remove( context, condition_id, CONDITION, &header );

    if ( error < 0 ) {
        return error;
    }

    condition = ( condition_t* )header;

    scheduler_lock();
    waitqueue_wake_up_all( &condition->waiters );
    scheduler_unlock();

    kfree( condition );

    return 0;
}

int condition_destroy( lock_id condition ) {
    return do_destroy_condition( &kernel_lock_context, condition );
}

int sys_condition_wait( lock_id condition, lock_id mutex ) {
    return do_wait_condition( current_process()->lock_context, condition, mutex, INFINITE_TIMEOUT );
}

int sys_condition_timedwait( lock_id condition, lock_id mutex, time_t* timeout ) {
    return do_wait_condition( current_process()->lock_context, condition, mutex, *timeout );
}

int sys_condition_signal( lock_id condition ) {
    return do_signal_condition( current_process()->lock_context, condition );
}

int sys_condition_broadcast( lock_id condition ) {
    return do_broadcast_condition( current_process()->lock_context, condition );
}

int sys_condition_create( const char* name ) {
    return do_create_condition( current_process()->lock_context, name );
}

int sys_condition_destroy( lock_id condition ) {
    return do_destroy_condition( current_process()->lock_context, condition );
}
