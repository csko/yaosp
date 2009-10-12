/* Common lock related functions and definitions
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
#include <lock/common.h>
#include <sched/scheduler.h>

#include <arch/pit.h>

int lock_wait_on( lock_context_t* context, thread_t* thread, lock_type_t type, lock_id id, waitqueue_t* queue ) {
    waitnode_t waitnode;
    lock_header_t* header;

    spinlock( &scheduler_lock );

    waitnode.thread = thread->id;
    waitnode.in_queue = false;

    waitqueue_add_node_tail( queue, &waitnode );

    thread->state = THREAD_WAITING;
    thread->blocking_semaphore = id;

    spinunlock( &scheduler_lock );
    spinunlock_enable( &context->lock );

    sched_preempt();

    spinlock_disable( &context->lock );

    thread->blocking_semaphore = -1;

    header = lock_context_get( context, id );

    if ( ( header == NULL ) ||
         ( header->type != type ) ) {
        return -EINVAL;
    }

    waitqueue_remove_node( queue, &waitnode );

    return 0;
}

int lock_timed_wait_on( lock_context_t* context, thread_t* thread, lock_type_t type,
                        lock_id id, waitqueue_t* queue, time_t wakeup_time ) {
    waitnode_t waitnode;
    waitnode_t sleepnode;
    lock_header_t* header;

    spinlock( &scheduler_lock );

    waitnode.thread = thread->id;
    waitnode.in_queue = false;

    waitqueue_add_node_tail( queue, &waitnode );

    sleepnode.thread = thread->id;
    sleepnode.wakeup_time = wakeup_time;
    sleepnode.in_queue = false;

    waitqueue_add_node( &sleep_queue, &sleepnode );

    thread->state = THREAD_WAITING;
    thread->blocking_semaphore = id;

    spinunlock( &scheduler_lock );
    spinunlock_enable( &context->lock );

    sched_preempt();

    spinlock_disable( &context->lock );

    thread->blocking_semaphore = -1;

    spinlock( &scheduler_lock );
    waitqueue_remove_node( &sleep_queue, &sleepnode );
    spinunlock( &scheduler_lock );

    header = lock_context_get( context, id );

    if ( ( header == NULL ) ||
         ( header->type != type ) )  {
        return -EINVAL;
    }

    waitqueue_remove_node( queue, &waitnode );

    return 0;
}
