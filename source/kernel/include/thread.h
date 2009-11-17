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
#include <signal.h>
#include <mm/region.h>
#include <lib/hashtable.h>

#include <arch/mm/config.h>

#define KERNEL_STACK_PAGES ( KERNEL_STACK_SIZE / PAGE_SIZE )
#define USER_STACK_PAGES   ( USER_STACK_SIZE / PAGE_SIZE )

enum {
    THREAD_UNKNOWN = 0,
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_SLEEPING,
    THREAD_ZOMBIE
};

enum {
    PRIORITY_HIGH = 31,
    PRIORITY_DISPLAY = 23,
    PRIORITY_NORMAL = 15,
    PRIORITY_LOW = 7,
    PRIORITY_IDLE = 0
};

struct process;

typedef struct thread {
    hashitem_t hash;

    struct thread* queue_next;

    thread_id id;
    thread_id parent_id;

    char* name;
    int state;
    int priority;
    int exit_code;
    int blocking_semaphore;

    struct process* process;

    /* Scheduling time stuffs */

    uint64_t quantum;
    uint64_t exec_time;
    uint64_t cpu_time;

    int in_system;
    uint64_t sys_time;
    uint64_t user_time;
    uint64_t prev_checkpoint;

    /* Kernel & user stack */

    uint32_t kernel_stack_pages;
    void* kernel_stack;
    void* kernel_stack_end;

    void* user_stack_end;
    memory_region_t* user_stack_region;

    void* syscall_stack;

    /* Signal handling */

    uint64_t pending_signals;
    uint64_t blocked_signals;
    struct sigaction signal_handlers[ _NSIG - 1 ];

    /* Architecture specific data */

    void* arch_data;
} thread_t;

typedef struct thread_info {
    thread_id id;
    char name[ MAX_THREAD_NAME_LENGTH ];
    int state;
    uint64_t cpu_time;
} thread_info_t;

typedef int thread_entry_t( void* arg );
typedef int thread_iter_callback_t( thread_t* thread, void* data );

extern hashtable_t thread_table;

thread_t* allocate_thread( const char* name, struct process* process, int priority, uint32_t kernel_stack_pages );
void destroy_thread( thread_t* thread );
int insert_thread( thread_t* thread );
int rename_thread( thread_t* thread, char* new_name );

void thread_exit( int exit_code );
void kernel_thread_exit( void );

/**
 * Creates a new kernel thread.
 *
 * @param name The name of the new thread
 * @param priority The priority of the new thread
 * @param entry The entry point of the new thread
 * @param arg The argument of the thread entry function
 * @param stack_size The stack size of the new thread (can be 0, in this case
 *                   the default size will be used
 * @return On success the id of the new thread is returned, otherwise a negative
 *         error code
 */
thread_id create_kernel_thread( const char* name, int priority, thread_entry_t* entry, void* arg, uint32_t stack_size );

/**
 * This method can be used to delay the execution of
 * the current thread for the specified number of microseconds
 * at least.
 *
 * @param microsecs The number of microseconds to delay the thread
 * @return On success 0 is returned
 */
int thread_sleep( uint64_t microsecs );

int do_wake_up_thread( thread_t* thread );

/**
 * Wakes up a waiting, sleeping or not yet started thread.
 *
 * @param id The id of the thread to wake up
 * @return On success 0 is returned
 */
int thread_wake_up( thread_id id );

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

thread_id sys_create_thread( const char* name, int priority, thread_entry_t* entry, void* arg, uint32_t user_stack_size );
int sys_exit_thread( int exit_code );
int sys_wake_up_thread( thread_id id );

thread_id sys_gettid( void );

uint32_t sys_get_thread_count_for_process( process_id id );
uint32_t sys_get_thread_info_for_process( process_id id, thread_info_t* info_table, uint32_t max_count );

int sys_sleep_thread( uint64_t* microsecs, uint64_t* remaining );

#ifdef ENABLE_DEBUGGER
int dbg_list_threads( const char* params );
int dbg_show_thread_info( const char* params );
int dbg_trace_thread( const char* params );
#endif /* ENABLE_DEBUGGER */

int init_threads( void );

#endif /* _THREAD_H_ */
