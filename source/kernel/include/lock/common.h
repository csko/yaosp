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

#ifndef _LOCK_COMMON_H_
#define _LOCK_COMMON_H_

#include <time.h>
#include <thread.h>
#include <sched/waitqueue.h>
#include <lock/context.h>

enum {
    LOCK_IGNORE_SIGNAL = ( 1 << 0 )
};

int lock_wait_on( lock_context_t* context, thread_t* thread, lock_type_t type, lock_id id, waitqueue_t* queue );
int lock_timed_wait_on( lock_context_t* context, thread_t* thread, lock_type_t type,
                        lock_id id, waitqueue_t* queue, time_t wakeup_time );

#endif /* _LOCK_COMMON_H_ */
