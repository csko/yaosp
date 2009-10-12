/* Condition variable definitions
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

#ifndef _LOCK_CONDITION_H_
#define _LOCK_CONDITION_H_

#include <thread.h>
#include <time.h>
#include <lock/context.h>
#include <lock/common.h>
#include <sched/waitqueue.h>

typedef struct condition {
    lock_header_t header;
    waitqueue_t waiters;
} condition_t;

int condition_wait( lock_id condition, lock_id mutex );
int condition_timedwait( lock_id condition, lock_id mutex, time_t timeout );
int condition_signal( lock_id condition );
int condition_broadcast( lock_id condition );

int condition_clone( condition_t* old, condition_t* new );
int condition_update( condition_t* condition, thread_id new_thread );

int sys_condition_wait( lock_id condition, lock_id mutex );
int sys_condition_timedwait( lock_id condition, lock_id mutex, time_t* timeout );
int sys_condition_signal( lock_id condition );
int sys_condition_broadcast( lock_id condition );
int sys_condition_create( const char* name );
int sys_condition_destroy( lock_id condition );

lock_id condition_create( const char* name );
int condition_destroy( lock_id condition );

#endif /* _LOCK_CONDITION_H_ */
