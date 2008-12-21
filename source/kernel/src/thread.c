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

#include <thread.h>
#include <config.h>
#include <errno.h>
#include <kernel.h>
#include <scheduler.h>
#include <smp.h>
#include <mm/kmalloc.h>
#include <mm/pages.h>
#include <lib/hashtable.h>
#include <lib/string.h>

#include <arch/thread.h>
#include <arch/spinlock.h>

#define MAX_THREAD_COUNT 1000000

static int thread_id_counter = 0;
static hashtable_t thread_table;

static thread_t* allocate_thread( const char* name, process_t* process ) {
    int error;
    thread_t* thread;

    thread = ( thread_t* )kmalloc( sizeof( thread_t ) );

    if ( thread == NULL ) {
        return NULL;
    }

    memset( thread, 0, sizeof( thread_t ) );

    thread->id = -1;
    thread->name = strdup( name );
    thread->process = process;

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

    error = arch_allocate_thread( thread );

    if ( error < 0 ) {
        free_pages( thread->kernel_stack, KERNEL_STACK_PAGES );
        kfree( thread->name );
        kfree( thread );
        return NULL;
    }

    thread->state = THREAD_READY;

    return thread;
}

void thread_exit( int exit_code ) {
    thread_t* thread;

    spinlock_disable( &scheduler_lock );

    thread = current_thread();
    thread->state = THREAD_ZOMBIE;

    spinunlock( &scheduler_lock );

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
        /* TODO: free the allocated thread structure */

        return error;
    }

    /* Get an unique ID to the new thread and add to the others */

    spinlock_disable( &scheduler_lock );

    do {
        thread->id = ( thread_id_counter++ ) % MAX_THREAD_COUNT;
    } while ( hashtable_get( &thread_table, ( const void* )thread->id ) != NULL );

    hashtable_add( &thread_table, ( hashitem_t* )thread );

    spinunlock_enable( &scheduler_lock );

    return thread->id;
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

thread_t* get_thread_by_id( thread_id id ) {
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
