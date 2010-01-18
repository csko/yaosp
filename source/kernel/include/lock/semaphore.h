/* Semaphore definitions
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

#ifndef _LOCK_SEMAPHORE_H_
#define _LOCK_SEMAPHORE_H_

#include <time.h>
#include <lock/context.h>
#include <lock/common.h>
#include <sched/waitqueue.h>

typedef struct semaphore {
    lock_header_t header;

    int count;
    int initial_count;
    waitqueue_t waiters;
} semaphore_t;

int semaphore_lock( lock_id semaphore, int count, int flags );
int semaphore_timedlock( lock_id semaphore, int count, int flags, time_t timeout );
int semaphore_unlock( lock_id semaphore, int count );
int semaphore_reset( lock_id semaphore );

lock_id semaphore_create( const char* name, int count );
int semaphore_destroy( lock_id semaphore );

#endif /* _LOCK_SEMAPHORE_H_ */
