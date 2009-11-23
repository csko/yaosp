/* Semaphore implementation
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
#include <smp.h>
#include <macros.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <lock/semaphore.h>
#include <lock/common.h>
#include <sched/scheduler.h>
#include <lib/string.h>

#include <arch/pit.h>

static int do_lock_semaphore( lock_context_t* context, lock_id semaphore_id, int count, int flags, uint64_t timeout ) {
    thread_t* thread;
    uint64_t wakeup_time;
    lock_header_t* header;
    semaphore_t* semaphore;

    wakeup_time = get_system_time() + timeout;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, semaphore_id );

    if ( ( header == NULL ) ||
         ( header->type != SEMAPHORE ) ) {
        spinunlock_enable( &context->lock );

        kprintf(
            WARNING,
            "do_lock_semaphore(): Invalid semaphore id or the specified lock is not a semaphore!\n"
        );

        return -EINVAL;
    }

    thread = current_thread();
    semaphore = ( semaphore_t* )header;

    while ( semaphore->count < count ) {
        int error;

        if ( timeout != INFINITE_TIMEOUT ) {
            if ( wakeup_time <= get_system_time() ) {
                spinunlock_enable( &context->lock );

                return -ETIME;
            }

            error = lock_timed_wait_on( context, thread, SEMAPHORE, semaphore_id, &semaphore->waiters, wakeup_time );
        } else {
            error = lock_wait_on( context, thread, SEMAPHORE, semaphore_id, &semaphore->waiters );
        }

        if ( error < 0 ) {
            return error;
        }

        if ( ( is_signal_pending( thread ) ) &&
             ( ( flags & LOCK_IGNORE_SIGNAL ) == 0 ) ) {
            spinunlock_enable( &context->lock );

            return -EINTR;
        }
    }

    semaphore->count -= count;

    spinunlock_enable( &context->lock );

    return 0;
}

int semaphore_lock( lock_id semaphore, int count, int flags ) {
    return do_lock_semaphore( &kernel_lock_context, semaphore, count, flags, INFINITE_TIMEOUT );
}

int semaphore_timedlock( lock_id semaphore, int count, int flags, time_t timeout ) {
    return do_lock_semaphore( &kernel_lock_context, semaphore, count, flags, timeout );
}

static int do_unlock_semaphore( lock_context_t* context, lock_id semaphore_id, int count ) {
    lock_header_t* header;
    semaphore_t* semaphore;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, semaphore_id );

    if ( ( header == NULL ) ||
         ( header->type != SEMAPHORE ) ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    semaphore = ( semaphore_t* )header;
    semaphore->count += count;

    spinlock( &scheduler_lock );
    waitqueue_wake_up_all( &semaphore->waiters );
    spinunlock( &scheduler_lock );

    spinunlock_enable( &context->lock );

    return 0;
}

int semaphore_unlock( lock_id semaphore, int count ) {
    return do_unlock_semaphore( &kernel_lock_context, semaphore, count );
}

static int do_create_semaphore( lock_context_t* context, const char* name, int count ) {
    int error;
    size_t name_length;
    lock_header_t* header;
    semaphore_t* semaphore;

    name_length = strlen( name );

    semaphore = ( semaphore_t* )kmalloc( sizeof( semaphore_t ) + name_length + 1 );

    if ( semaphore == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* Initialize the lock header */

    header = ( lock_header_t* )semaphore;

    header->type = SEMAPHORE;
    header->name = ( char* )( semaphore + 1 );
    memcpy( header->name, name, name_length + 1 );

    /* Initialize semaphore specific fields */

    semaphore->count = count;

    error = init_waitqueue( &semaphore->waiters );

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
    kfree( semaphore );

 error1:
    return error;
}

lock_id semaphore_create( const char* name, int count ) {
    return do_create_semaphore( &kernel_lock_context, name, count );
}

static int do_destroy_semaphore( lock_context_t* context, lock_id semaphore_id ) {
    int error;
    lock_header_t* header;
    semaphore_t* semaphore;

    error = lock_context_remove( context, semaphore_id, SEMAPHORE, &header );

    if ( error < 0 ) {
        return error;
    }

    semaphore = ( semaphore_t* )header;

    scheduler_lock();
    waitqueue_wake_up_all( &semaphore->waiters );
    scheduler_unlock();

    kfree( semaphore );

    return 0;
}

int semaphore_destroy( lock_id semaphore ) {
    return do_destroy_semaphore( &kernel_lock_context, semaphore );
}
