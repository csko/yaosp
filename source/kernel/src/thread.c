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

#include <thread.h>
#include <config.h>
#include <errno.h>
#include <kernel.h>
#include <scheduler.h>
#include <smp.h>
#include <waitqueue.h>
#include <macros.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <mm/pages.h>
#include <lib/hashtable.h>
#include <lib/string.h>

#include <arch/pit.h> /* get_system_time() */
#include <arch/thread.h>
#include <arch/spinlock.h>
#include <arch/interrupt.h>

typedef struct thread_info_iter_data {
    uint32_t curr_index;
    uint32_t max_count;
    thread_info_t* info_table;
} thread_info_iter_data_t;

static int thread_id_counter = 0;
static hashtable_t thread_table;

static thread_id thread_cleaner;
static thread_t* thread_cleaner_list;
static semaphore_id thread_cleaner_sync;

thread_t* allocate_thread( const char* name, process_t* process, int priority, uint32_t kernel_stack_pages ) {
    int error;
    thread_t* thread;

    thread = ( thread_t* )kmalloc( sizeof( thread_t ) );

    if ( thread == NULL ) {
        goto error1;
    }

    memset( thread, 0, sizeof( thread_t ) );

    thread->name = strdup( name );

    if ( thread->name == NULL ) {
        goto error2;
    }

    thread->kernel_stack_pages = kernel_stack_pages;
    thread->kernel_stack = ( void* )alloc_pages( kernel_stack_pages, MEM_COMMON );

    if ( thread->kernel_stack == NULL ) {
        goto error3;
    }

    thread->kernel_stack_end = ( uint8_t* )thread->kernel_stack + ( kernel_stack_pages * PAGE_SIZE );

    error = arch_allocate_thread( thread );

    if ( error < 0 ) {
        goto error4;
    }

    thread->id = -1;
    thread->state = THREAD_NEW;
    thread->priority = priority;
    thread->process = process;
    thread->user_stack_end = NULL;
    thread->user_stack_region = -1;

    atomic_inc( &process->thread_count );

    reset_thread_quantum( thread );

    return thread;

error4:
    free_pages( thread->kernel_stack, kernel_stack_pages );

error3:
    kfree( thread->name );

error2:
    kfree( thread );

error1:
    return NULL;
}

void destroy_thread( thread_t* thread ) {
    /* Destroy the architecture dependent part of the thread */

    arch_destroy_thread( thread );

    /* Delete the userspace stack region */

    if ( thread->user_stack_region >= 0 ) {
        delete_region( thread->user_stack_region );
        thread->user_stack_region = -1;
    }

    /* Free the kernel stack */

    free_pages( thread->kernel_stack, thread->kernel_stack_pages );

    /* Destroy the process as well if this is the last thread */

    if ( atomic_dec_and_test( &thread->process->thread_count ) ) {
        /* Remove the process from the hashtable */

        spinlock_disable( &scheduler_lock );

        remove_process( thread->process );

        spinunlock_enable( &scheduler_lock );

        /* Destroy the process */

        destroy_process( thread->process );
    }

    /* Free other resources allocated by the thread */

    kfree( thread->name );
    kfree( thread );
}

int insert_thread( thread_t* thread ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );

    do {
        thread->id = thread_id_counter++;

        if ( thread_id_counter < 0 ) {
            thread_id_counter = 0;
        }
    } while ( hashtable_get( &thread_table, ( const void* )thread->id ) != NULL );

    hashtable_add( &thread_table, ( hashitem_t* )thread );

    return 0;
}

int rename_thread( thread_t* thread, char* new_name ) {
    char* name;

    ASSERT( spinlock_is_locked( &scheduler_lock ) );

    name = strdup( new_name );

    if ( name == NULL ) {
        return -ENOMEM;
    }

    kfree( thread->name );
    thread->name = name;

    return 0;
}

void thread_exit( int exit_code ) {
    thread_t* thread;

    /* Disable interrupts to make sure the timer interrupt won't preempt us */

    disable_interrupts();

    /* Lock the scheduler. We set the thread state to zombie, so the scheduler
       won't try to run it again. We also add the threadt to the cleaner list */

    spinlock( &scheduler_lock );

    thread = current_thread();
    thread->state = THREAD_ZOMBIE;

    thread->queue_next = thread_cleaner_list;
    thread_cleaner_list = thread;

    spinunlock( &scheduler_lock );

    /* Tell the thread cleaner that we have something to clean */

    UNLOCK( thread_cleaner_sync );

    /* Make sure we don't try to execute this thread anymore */

    while ( 1 ) {
        sched_preempt();
    }
}

void kernel_thread_exit( void ) {
    thread_exit( 0 );
}

thread_id create_kernel_thread( const char* name, int priority, thread_entry_t* entry, void* arg, uint32_t stack_size ) {
    int error;
    thread_t* thread;
    process_t* kernel_process;

    /* Get the kernel process */

    spinlock_disable( &scheduler_lock );

    kernel_process = get_process_by_id( 0 );

    spinunlock_enable( &scheduler_lock );

    if ( kernel_process == NULL ) {
        return -EINVAL;
    }

    /* Calculate stack size */

    stack_size = PAGE_ALIGN( stack_size );

    if ( stack_size == 0 ) {
        stack_size = KERNEL_STACK_PAGES;
    } else {
        stack_size /= PAGE_SIZE;
    }

    /* Allocate a new thread */

    thread = allocate_thread( name, kernel_process, priority, stack_size );

    if ( thread == NULL ) {
        return -ENOMEM;
    }

    /* Initialize the architecture dependent part of the thread */

    error = arch_create_kernel_thread( thread, ( void* )entry, arg );

    if ( error < 0 ) {
        destroy_thread( thread );
        return error;
    }

    /* Get an unique ID to the new thread and add to the others */

    spinlock_disable( &scheduler_lock );

    error = insert_thread( thread );

    if ( error >= 0 ) {
        error = thread->id;
    }

    spinunlock_enable( &scheduler_lock );

    return error;
}

int sleep_thread( uint64_t microsecs ) {
    int error;
    thread_t* thread;
    waitnode_t node;

    ASSERT( !is_interrupts_disabled() );

    node.wakeup_time = get_system_time() + microsecs;

    spinlock_disable( &scheduler_lock );

    thread = current_thread();

    thread->state = THREAD_SLEEPING;
    node.thread = thread->id;

    waitqueue_add_node( &sleep_queue, &node );

    spinunlock_enable( &scheduler_lock );

    sched_preempt();

    spinlock_disable( &scheduler_lock );

    waitqueue_remove_node( &sleep_queue, &node );

    spinunlock_enable( &scheduler_lock );

    if ( get_system_time() < node.wakeup_time ) {
        error = -ETIME;
    } else {
        error = 0;
    }

    return error;
}

static int thread_info_process_filter( hashitem_t* item, void* data ) {
    process_id id;
    thread_t* thread;

    id = *( ( process_id* )data );
    thread = ( thread_t* )item;

    if ( thread->process->id != id ) {
        return -EINVAL;
    }

    return 0;
}

static int get_thread_info_iterator( hashitem_t* item, void* _data ) {
    thread_t* thread;
    thread_info_t* info;
    thread_info_iter_data_t* data;

    thread = ( thread_t* )item;
    data = ( thread_info_iter_data_t* )_data;

    if ( data->curr_index >= data->max_count ) {
        return 0;
    }

    info = ( thread_info_t* )&data->info_table[ data->curr_index ];

    info->id = thread->id;
    strncpy( info->name, thread->name, MAX_THREAD_NAME_LENGTH );
    info->name[ MAX_THREAD_NAME_LENGTH - 1 ] = 0;
    info->cpu_time = 0;

    data->curr_index++;

    return 0;
}

uint32_t sys_get_thread_count_for_process( process_id id ) {
    uint32_t count;

    spinlock_disable( &scheduler_lock );

    count = hashtable_get_filtered_item_count( &thread_table, thread_info_process_filter, ( void* )&id );

    spinunlock_enable( &scheduler_lock );

    return count;
}

uint32_t sys_get_thread_info_for_process( process_id id, thread_info_t* info_table, uint32_t max_count ) {
    thread_info_iter_data_t data;

    data.curr_index = 0;
    data.max_count = max_count;
    data.info_table = info_table;

    spinlock_disable( &scheduler_lock );

    hashtable_filtered_iterate( &thread_table, get_thread_info_iterator, ( void* )&data, thread_info_process_filter, ( void* )&id );

    spinunlock_enable( &scheduler_lock );

    return data.curr_index;
}

int sys_sleep_thread( uint64_t* microsecs ) {
    return sleep_thread( *microsecs );
}

int wake_up_thread( thread_id id ) {
    int error;
    thread_t* thread;

    spinlock_disable( &scheduler_lock );

    thread = get_thread_by_id( id );

    if ( ( thread != NULL ) &&
         ( ( thread->state == THREAD_NEW ) ||
           ( thread->state == THREAD_WAITING ) ||
           ( thread->state == THREAD_SLEEPING ) ) ) {
        add_thread_to_ready( thread );
        error = 0;
    } else {
        error = -EINVAL;
    }

    spinunlock_enable( &scheduler_lock );

    return error;
}

uint32_t get_thread_count( void ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );
    return hashtable_get_item_count( &thread_table );
}

thread_t* get_thread_by_id( thread_id id ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );
    return ( thread_t* )hashtable_get( &thread_table, ( const void* )id );
}

static void* thread_key( hashitem_t* item ) {
    thread_t* thread;

    thread = ( thread_t* )item;

    return ( void* )thread->id;
}

static uint32_t thread_hash( const void* key ) {
    return ( uint32_t )key;
}

static bool thread_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

__init int init_threads( void ) {
    int error;

    error = init_hashtable(
        &thread_table,
        256,
        thread_key,
        thread_hash,
        thread_compare
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int thread_cleaner_entry( void* arg ) {
    thread_t* thread;

    while ( 1 ) {
        /* Wait until a thread is added to the cleaner list */

        LOCK( thread_cleaner_sync );

        /* Pop the first thread from the cleaner list. We also remove
           the thread from the global hashtable here because now we
           own the scheduler lock so with this solution we don't have
           to lock/unlock it again. */

        spinlock_disable( &scheduler_lock );

        thread = thread_cleaner_list;

        if ( thread != NULL ) {
            thread_cleaner_list = thread->queue_next;

            hashtable_remove( &thread_table, ( const void* )thread->id );
        }

        spinunlock_enable( &scheduler_lock );

        if ( thread == NULL ) {
            continue;
        }

        /* Free the resources allocated by this thread */

        destroy_thread( thread );
    }

    return 0;
}

__init int init_thread_cleaner( void ) {
    thread_cleaner_list = NULL;

    thread_cleaner_sync = create_semaphore( "thread cleaner sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( thread_cleaner_sync < 0 ) {
        goto error1;
    }

    thread_cleaner = create_kernel_thread( "thread cleaner", PRIORITY_LOW, thread_cleaner_entry, NULL, 0 );

    if ( thread_cleaner < 0 ) {
        goto error2;
    }

    wake_up_thread( thread_cleaner );

    return 0;

error2:
    delete_semaphore( thread_cleaner_sync );

error1:
    return -1;
}
