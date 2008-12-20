/* Thread implementation
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

#ifndef _THREAD_H_
#define _THREAD_H_

#include <types.h>
#include <lib/hashtable.h>

typedef int thread_id;

enum {
    THREAD_UNKNOWN = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_SLEEPING,
};

typedef struct thread {
    hashitem_t hash;

    struct thread* queue_next;

    thread_id id;
    char* name;
    int state;

    void* kernel_stack;

    void* arch_data;
} thread_t;

typedef int thread_entry_t( void* arg );

void kernel_thread_exit( void );

thread_id create_kernel_thread( const char* name, thread_entry_t* entry, void* arg );

thread_t* get_thread_by_id( thread_id id );

int init_threads( void );

#endif // _THREAD_H_
