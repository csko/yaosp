/* Thread implementation
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
#include <config.h>
#include <mm/region.h>
#include <lib/hashtable.h>

#include <arch/mm/config.h>

#define KERNEL_STACK_PAGES ( KERNEL_STACK_SIZE / PAGE_SIZE )

typedef int thread_id;

enum {
    THREAD_UNKNOWN = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_SLEEPING,
    THREAD_ZOMBIE
};

struct process;

typedef struct thread {
    hashitem_t hash;

    struct thread* queue_next;

    thread_id id;
    char* name;
    int state;

    uint64_t quantum;
    uint64_t exec_time;

    uint64_t cpu_time;

    struct process* process;

    void* kernel_stack;
    void* kernel_stack_end;
    void* user_stack_end;
    region_id user_stack_region;
    void* syscall_stack;

    void* arch_data;
} thread_t;

typedef int thread_entry_t( void* arg );
typedef int thread_iter_callback_t( thread_t* thread, void* data );

thread_t* allocate_thread( const char* name, struct process* process );
void destroy_thread( thread_t* thread );
int insert_thread( thread_t* thread );
int rename_thread( thread_t* thread, char* new_name );

void thread_exit( int exit_code );
void kernel_thread_exit( void );

thread_id create_kernel_thread( const char* name, thread_entry_t* entry, void* arg );

/**
 * This method can be used to delay the execution of
 * the current thread for the specified number of microseconds
 * at least.
 *
 * @param microsecs The number of microseconds to delay the thread
 * @return On success 0 is returned
 */
int sleep_thread( uint64_t microsecs );
int wake_up_thread( thread_id id );

/**
 * Returnes the total number of created threads.
 *
 * @return The total number of threads
 */
uint32_t get_thread_count( void );

/**
 * Returns the thread associated with the given ID.
 * You have to call this method with a locked scheduler!
 *
 * @param id The ID of the thread to look for
 * @return On success a non-NULL pointer is returned
 *         pointing to the thread with the given ID,
 *         otherwise NULL
 */
thread_t* get_thread_by_id( thread_id id );

int thread_table_iterate( thread_iter_callback_t* callback, void* data );

int sys_sleep_thread( uint64_t* microsecs );

int init_threads( void );
int init_thread_cleaner( void );

#endif // _THREAD_H_
