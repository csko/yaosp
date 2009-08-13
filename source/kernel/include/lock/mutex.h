/* Mutex definitions
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

#ifndef _LOCK_MUTEX_H_
#define _LOCK_MUTEX_H_

#include <thread.h>
#include <waitqueue.h>
#include <time.h>
#include <lock/context.h>

enum mutex_flags {
    MUTEX_NONE = 0,
    MUTEX_RECURSIVE = ( 1 << 0 )
};

typedef struct mutex {
    lock_header_t header;

    int flags;
    int recursive_count;

    thread_id holder;
    waitqueue_t waiters;
} mutex_t;

int mutex_lock( lock_id mutex );
int mutex_trylock( lock_id mutex );
int mutex_timedlock( lock_id mutex, time_t timeout );
int mutex_unlock( lock_id mutex );

int mutex_clone( mutex_t* old, mutex_t* new );
int mutex_update( mutex_t* mutex, thread_id new_thread );

int sys_mutex_lock( lock_id mutex );
int sys_mutex_trylock( lock_id mutex );
int sys_mutex_timedlock( lock_id mutex, time_t* timeout );
int sys_mutex_unlock( lock_id mutex );
int sys_mutex_create( const char* name, int flags );
int sys_mutex_destroy( lock_id mutex );

lock_id mutex_create( const char* name, int flags );
int mutex_destroy( lock_id mutex );

#endif /* _LOCK_MUTEX_H_ */
