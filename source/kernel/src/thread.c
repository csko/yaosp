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

static int thread_id_counter = 0;
static hashtable_t thread_table;

static thread_id thread_cleaner;
static thread_t* thread_cleaner_list;
static semaphore_id thread_cleaner_sync;

thread_t* allocate_thread( const char* name, process_t* process ) {
    int error;
    thread_t* thread;

    thread = ( thread_t* )kmalloc( sizeof( thread_t ) );

    if ( thread == NULL ) {
        return NULL;
    }

    memset( thread, 0, sizeof( thread_t ) );

    thread->name = strdup( name );

    if ( thread->name == NULL ) {
        kfree( thread );
        return NULL;
    }

    thread->kernel_stack = ( register_t* )alloc_pages( KERNEL_STACK_PAGES );

    if ( thread->kernel_stack == NULL ) {
        kfree( thread->name );
        kfree( thread );
        return NULL;
    }

    thread->kernel_stack_end = ( uint8_t* )thread->kernel_stack + ( KERNEL_STACK_PAGES * PAGE_SIZE );

    error = arch_allocate_thread( thread );

    if ( error < 0 ) {
        free_pages( thread->kernel_stack, KERNEL_STACK_PAGES );
        kfree( thread->name );
        kfree( thread );
        return NULL;
    }

    thread->id = -1;
    thread->state = THREAD_READY;
    thread->process = process;
    thread->user_stack_end = NULL;
    thread->user_stack_region = -1;

    atomic_inc( &process->thread_count );

    reset_thread_quantum( thread );

    return thread;
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

    free_pages( thread->kernel_stack, KERNEL_STACK_PAGES );

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

thread_id create_kernel_thread( const char* name, thread_entry_t* entry, void* arg ) {
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

    /* Allocate a new thread */

    thread = allocate_thread( name, kernel_process );

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

    if ( get_system_time() < node.wakeup_time ) {
        return -ETIME;
    }

    return 0;
}

int wake_up_thread( thread_id id ) {
    int error;
    thread_t* thread;

    spinlock_disable( &scheduler_lock );

    thread = get_thread_by_id( id );

    if ( thread != NULL ) {
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

int init_threads( void ) {
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

int init_thread_cleaner( void ) {
    thread_cleaner_list = NULL;

    thread_cleaner_sync = create_semaphore( "thread cleaner sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( thread_cleaner_sync < 0 ) {
        goto error1;
    }

    thread_cleaner = create_kernel_thread( "thread cleaner", thread_cleaner_entry, NULL );

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
