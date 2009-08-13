/* Locking context definitions
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

#ifndef _LOCK_CONTEXT_H_
#define _LOCK_CONTEXT_H_

#include <lib/hashtable.h>

#include <arch/spinlock.h>

typedef enum lock_type {
    MUTEX,
    SEMAPHORE,
    CONDITION,
    RWLOCK,
    LOCK_COUNT
} lock_type_t;

typedef int lock_id;

typedef struct lock_header {
    hashitem_t hash;
    lock_type_t type;
    lock_id id;
    char* name;
} lock_header_t;

typedef struct lock_context {
    spinlock_t lock;
    lock_id next_lock_id;
    hashtable_t lock_table;
} lock_context_t;

extern lock_context_t kernel_lock_context;

lock_header_t* lock_context_get( lock_context_t* context, lock_id id );
int lock_context_insert( lock_context_t* context, lock_header_t* lock );
int lock_context_remove( lock_context_t* context, lock_id id, lock_type_t type, lock_header_t** lock );

lock_context_t* lock_context_clone( lock_context_t* old_context );
int lock_context_update( lock_context_t* context, thread_id new_thread );
int lock_context_make_empty( lock_context_t* context );

int lock_context_init( lock_context_t* context );
int lock_context_destroy( lock_context_t* context );

int init_locking( void );

#endif /* _LOCK_CONTEXT_H_ */
