/* Semaphore functions
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

#ifndef _YAOSP_SEMAPHORE_H_
#define _YAOSP_SEMAPHORE_H_

#include <sys/types.h>

#define LOCK(id) \
    lock_semaphore( id, 1, INFINITE_TIMEOUT )

#define UNLOCK(id) \
    unlock_semaphore( id, 1 )

typedef int semaphore_id;

typedef enum semaphore_type {
    SEMAPHORE_BINARY,
    SEMAPHORE_COUNTING
} semaphore_type_t;

typedef enum semaphore_flags {
    SEMAPHORE_GLOBAL = ( 1 << 0 ),
    SEMAPHORE_RECURSIVE = ( 1 << 1 )
} semaphore_flags_t;

semaphore_id create_semaphore( const char* name, semaphore_type_t type, semaphore_flags_t flags, int count );
int delete_semaphore( semaphore_id id );
int lock_semaphore( semaphore_id id, int count, uint64_t timeout );
int unlock_semaphore( semaphore_id id, int count );

#endif // _YAOSP_SEMAPHORE_H_
