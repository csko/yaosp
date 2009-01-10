/* Semaphore implementation
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include <types.h>
#include <waitqueue.h>
#include <lib/hashtable.h>

#include <arch/spinlock.h>

#define INFINITE_TIMEOUT 18446744073709551615ULL
#define LOCK(sem) lock_semaphore( sem, 1, INFINITE_TIMEOUT )
#define UNLOCK(sem) unlock_semaphore( sem, 1 )

typedef enum semaphore_type {
    SEMAPHORE_BINARY,
    SEMAPHORE_COUNTING
} semaphore_type_t;

typedef enum semaphore_flags {
    SEMAPHORE_GLOBAL = ( 1 << 0 ),
    SEMAPHORE_RECURSIVE = ( 1 << 1 )
} semaphore_flags_t;

typedef int semaphore_id;

typedef struct semaphore {
    hashitem_t hash;

    semaphore_id id;
    char* name;

    int count;
    semaphore_type_t type;
    uint32_t flags;

    waitqueue_t waiters;
} semaphore_t;

typedef struct semaphore_context {
    spinlock_t lock;
    int semaphore_id_counter;
    hashtable_t semaphore_table;
} semaphore_context_t;

extern semaphore_context_t kernel_semaphore_context;

/**
 * This functions creates a new kernel semaphore.
 *
 * @param name The name of the semaphore
 * @param type The type of the semaphore
 * @param flags Semaphore flags
 * @param count The initial count of semaphore (used only for counting semaphores)
 * @return On success a non-negative semaphore ID is returned
 */
semaphore_id create_semaphore( const char* name, semaphore_type_t type, uint32_t flags, int count );
int delete_semaphore( semaphore_id id );
int lock_semaphore( semaphore_id id, int count, uint64_t timeout );
int unlock_semaphore( semaphore_id id, int count );

semaphore_id sys_create_semaphore( const char* name, semaphore_type_t type, uint32_t flags, int count );
int sys_delete_semaphore( semaphore_id id );
int sys_lock_semaphore( semaphore_id id, int count, uint64_t* timeout );
int sys_unlock_semaphore( semaphore_id id, int count );

int init_semaphore_context( semaphore_context_t* context );
semaphore_context_t* semaphore_context_clone( semaphore_context_t* old_context );

int init_semaphores( void );

#endif // _SEMAPHORE_H_
