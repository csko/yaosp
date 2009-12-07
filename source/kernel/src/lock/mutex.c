/* Mutex implementation
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
#include <console.h>
#include <smp.h>
#include <macros.h>
#include <debug.h>
#include <mm/kmalloc.h>
#include <lock/mutex.h>
#include <lock/common.h>
#include <sched/scheduler.h>
#include <lib/string.h>

#include <arch/pit.h>

int do_acquire_mutex( lock_context_t* context, mutex_t* mutex, thread_t* thread,
                      time_t timeout, bool try_lock, int flags ) {
    int error;
    lock_id mutex_id;
    uint64_t wakeup_time;

    mutex_id = mutex->header.id;
    wakeup_time = get_system_time() + timeout;

    while ( mutex->holder != -1 ) {
        /* Handle recursive mutexes */

        if ( ( mutex->holder == thread->id ) &&
             ( mutex->flags & MUTEX_RECURSIVE ) ) {
            break;
        }

        /* Handle mutex_trylock() */

        if ( try_lock ) {
            return -EBUSY;
        }

        if ( ( timeout != INFINITE_TIMEOUT ) &&
             ( wakeup_time <= get_system_time() ) ) {
            return -ETIME;
        }

        /* Wait for the mutex to be released */

        if ( timeout != INFINITE_TIMEOUT ) {
            uint64_t now;

            error = lock_timed_wait_on( context, thread, MUTEX, mutex->header.id, &mutex->waiters, wakeup_time );

            if ( ( error == 0 ) &&
                 ( mutex->holder != -1 ) ) {
                now = get_system_time();

                if ( now < wakeup_time ) {
                    error = -EINTR;
                } else {
                    return -ETIME;
                }
            }
        } else {
            error = lock_wait_on( context, thread, MUTEX, mutex->header.id, &mutex->waiters );
        }

        if ( ( mutex->holder != -1 ) &&
             ( is_signal_pending( thread ) ) &&
             ( ( flags & LOCK_IGNORE_SIGNAL ) == 0 ) ) {
            continue;
        }

        if ( error < 0 ) {
            return error;
        }
    }

    mutex->holder = thread->id;
    mutex->recursive_count++;

    return 0;
}

static int do_lock_mutex( lock_context_t* context, lock_id mutex_id, bool try_lock, time_t timeout, int flags ) {
    int error;
    mutex_t* mutex;
    thread_t* thread;
    lock_header_t* header;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, mutex_id );

    if ( __unlikely( ( header == NULL ) ||
                     ( header->type != MUTEX ) ) ) {
        spinunlock_enable( &context->lock );

        kprintf(
            WARNING,
            "do_lock_mutex(): Invalid mutex id or the specified lock is not a mutex!\n"
        );

        return -EINVAL;
    }

    mutex = ( mutex_t* )header;
    thread = current_thread();

    if ( ( mutex->holder == thread->id ) &&
         ( timeout == INFINITE_TIMEOUT ) &&
         ( !try_lock ) &&
         ( ( mutex->flags & MUTEX_RECURSIVE ) == 0 ) ) {
        kprintf(
            WARNING,
            "Detected a deadlock while %s:%s tried to lock '%s'!\n",
            thread->process->name,
            thread->name,
            header->name
        );
        debug_print_stack_trace();

        spinunlock_enable( &context->lock );

        return -EDEADLK;
    }

    error = do_acquire_mutex( context, mutex, thread, timeout, try_lock, flags );

    spinunlock_enable( &context->lock );

    return error;
}

int mutex_lock( lock_id mutex, int flags ) {
    return do_lock_mutex( &kernel_lock_context, mutex, false, INFINITE_TIMEOUT, flags );
}

int mutex_trylock( lock_id mutex, int flags ) {
    return do_lock_mutex( &kernel_lock_context, mutex, true, INFINITE_TIMEOUT, flags );
}

int mutex_timedlock( lock_id mutex, time_t timeout, int flags ) {
    return do_lock_mutex( &kernel_lock_context, mutex, false, timeout, flags );
}

void do_release_mutex( mutex_t* mutex ) {
    /* Release the mutex */

    if ( --mutex->recursive_count == 0 ) {
        mutex->holder = -1;

        /* Wake up one waiter */

        spinlock( &scheduler_lock );
        waitqueue_wake_up_head( &mutex->waiters, 1 );
        spinunlock( &scheduler_lock );
    }
}

static int do_mutex_unlock( lock_context_t* context, lock_id mutex_id ) {
    mutex_t* mutex;
    thread_t* thread;
    lock_header_t* header;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, mutex_id );

    if ( __unlikely( ( header == NULL ) ||
                     ( header->type != MUTEX ) ) ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    mutex = ( mutex_t* )header;
    thread = current_thread();

    /* Make sure that the mutex is owned by the current thread */

    if ( mutex->holder != thread->id ) {
        kprintf(
            WARNING,
            "Tried to unlock a mutex that is taken by another thread!\n"
        );

        spinunlock_enable( &context->lock );

        return -EPERM;
    }

    /* Release the mutex */

    do_release_mutex( mutex );

    spinunlock_enable( &context->lock );

    return 0;
}

int mutex_unlock( lock_id mutex ) {
    return do_mutex_unlock( &kernel_lock_context, mutex );
}

int mutex_clone( mutex_t* old, mutex_t* new ) {
    int error;

    error = init_waitqueue( &new->waiters );

    if ( error < 0 ) {
        return error;
    }

    new->flags = old->flags;

    if ( old->holder == current_thread()->id ) {
        new->holder = 0; /* this will tell lock_context_update() to do the update :) */
        new->recursive_count = old->recursive_count;
    } else {
        new->holder = -1;
        new->recursive_count = 0;
    }

    return 0;
}

int mutex_update( mutex_t* mutex, thread_id new_thread ) {
    if ( mutex->holder == 0 ) {
        mutex->holder = new_thread;
    }

    return 0;
}

static int do_mutex_is_locked( lock_context_t* context, lock_id mutex_id ) {
    int locked;
    mutex_t* mutex;
    thread_t* thread;
    lock_header_t* header;

    spinlock_disable( &context->lock );

    header = lock_context_get( context, mutex_id );

    if ( __unlikely( ( header == NULL ) ||
                     ( header->type != MUTEX ) ) ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    mutex = ( mutex_t* )header;
    thread = current_thread();

    locked = ( mutex->holder == thread->id );

    spinunlock_enable( &context->lock );

    return locked;
}

int mutex_is_locked( lock_id mutex ) {
    return do_mutex_is_locked( &kernel_lock_context, mutex );
}

static int do_create_mutex( lock_context_t* context, const char* name, int flags ) {
    int error;
    mutex_t* mutex;
    size_t name_length;
    lock_header_t* header;

    name_length = strlen( name );

    mutex = ( mutex_t* )kmalloc( sizeof( mutex_t ) + name_length + 1 );

    if ( mutex == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* Initialize the lock header */

    header = ( lock_header_t* )mutex;

    header->type = MUTEX;
    header->name = ( char* )( mutex + 1 );
    memcpy( header->name, name, name_length + 1 );

    /* Initialize mutex specific fields */

    mutex->holder = -1;
    mutex->flags = flags;
    mutex->recursive_count = 0;

    error = init_waitqueue( &mutex->waiters );

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
    kfree( mutex );

 error1:
    return error;
}

lock_id mutex_create( const char* name, int flags ) {
    return do_create_mutex( &kernel_lock_context, name, flags );
}

static int do_destroy_mutex( lock_context_t* context, lock_id mutex_id ) {
    int error;
    mutex_t* mutex;
    lock_header_t* header;

    error = lock_context_remove( context, mutex_id, MUTEX, &header );

    if ( error < 0 ) {
        return error;
    }

    mutex = ( mutex_t* )header;

    scheduler_lock();
    waitqueue_wake_up_all( &mutex->waiters );
    scheduler_unlock();

    kfree( mutex );

    return 0;
}

int mutex_destroy( lock_id mutex ) {
    return do_destroy_mutex( &kernel_lock_context, mutex );
}

int sys_mutex_lock( lock_id mutex ) {
    return do_lock_mutex( current_process()->lock_context, mutex, false, INFINITE_TIMEOUT, 0 );
}

int sys_mutex_trylock( lock_id mutex ) {
    return do_lock_mutex( current_process()->lock_context, mutex, true, INFINITE_TIMEOUT, 0 );
}

int sys_mutex_timedlock( lock_id mutex, time_t* timeout ) {
    return do_lock_mutex( current_process()->lock_context, mutex, false, *timeout, 0 );
}

int sys_mutex_unlock( lock_id mutex ) {
    return do_mutex_unlock( current_process()->lock_context, mutex );
}

int sys_mutex_create( const char* name, int flags ) {
    return do_create_mutex( current_process()->lock_context, name, flags );
}

int sys_mutex_destroy( lock_id mutex ) {
    return do_destroy_mutex( current_process()->lock_context, mutex );
}
