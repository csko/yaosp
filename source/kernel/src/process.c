/* Process implementation
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

#include <process.h>
#include <errno.h>
#include <semaphore.h>
#include <smp.h>
#include <scheduler.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <mm/context.h>
#include <vfs/vfs.h>
#include <lib/hashtable.h>
#include <lib/string.h>

typedef struct process_info_iter_data {
    uint32_t curr_index;
    uint32_t max_count;
    process_info_t* info_table;
} process_info_iter_data_t;

static int process_id_counter = 0;
static hashtable_t process_table;

process_t* allocate_process( char* name ) {
    process_t* process;

    process = ( process_t* )kmalloc( sizeof( process_t ) );

    if ( process == NULL ) {
        goto error1;
    }

    memset( process, 0, sizeof( process_t ) );

    process->name = strdup( name );

    if ( process->name == NULL ) {
        goto error2;
    }

    process->lock = create_semaphore( "process lock", SEMAPHORE_BINARY, 0, 1 );

    if ( process->lock < 0 ) {
        goto error3;
    }

    process->waiters = create_semaphore( "proc exit waiters", SEMAPHORE_COUNTING, 0, 0 );

    if ( process->waiters < 0 ) {
        goto error4;
    }

    process->id = -1;
    process->heap_region = -1;

    atomic_set( &process->thread_count, 0 );

    return process;

error4:
    delete_semaphore( process->lock );

error3:
    kfree( process->name );

error2:
    kfree( process );

error1:
    return NULL;
}

void destroy_process( process_t* process ) {
    /* NOTE: We don't delete the heap region here because the call to
             the memory_context_delete_regions() will do it for us! */

    /* Destroy the memory context */

    if ( process->memory_context != NULL ) {
        memory_context_delete_regions( process->memory_context, false );
        memory_context_destroy( process->memory_context );
    }

    /* Destroy the I/O context */

    if ( process->io_context != NULL ) {
        destroy_io_context( process->io_context );
    }

    /* Destroy the semaphore context */

    if ( process->semaphore_context != NULL ) {
        destroy_semaphore_context( process->semaphore_context );
    }

    /* Delete other resources allocated by the process */

    delete_semaphore( process->lock );
    delete_semaphore( process->waiters );
    kfree( process->name );
    kfree( process );
}

int insert_process( process_t* process ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );

    do {
        process->id = process_id_counter++;

        if ( process_id_counter < 0 ) {
            process_id_counter = 0;
        }
    } while ( hashtable_get( &process_table, ( const void* )process->id ) != NULL );

    hashtable_add( &process_table, ( hashitem_t* )process );

    return 0;
}

void remove_process( process_t* process ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );
    hashtable_remove( &process_table, ( const void* )process->id );
}

int rename_process( process_t* process, char* new_name ) {
    char* name;

    ASSERT( spinlock_is_locked( &scheduler_lock ) );

    name = strdup( new_name );

    if ( name == NULL ) {
        return -ENOMEM;
    }

    kfree( process->name );
    process->name = name;

    return 0;
}

uint32_t get_process_count( void ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );
    return hashtable_get_item_count( &process_table );
}

process_t* get_process_by_id( process_id id ) {
    ASSERT( spinlock_is_locked( &scheduler_lock ) );
    return ( process_t* )hashtable_get( &process_table, ( const void* )id );
}

uint32_t sys_get_process_count( void ) {
    uint32_t result;

    spinlock_disable( &scheduler_lock );

    result = hashtable_get_item_count( &process_table );

    spinunlock_enable( &scheduler_lock );

    return result;
}

static int get_process_info_iterator( hashitem_t* item, void* _data ) {
    process_t* process;
    process_info_t* info;
    process_info_iter_data_t* data;

    process = ( process_t* )item;
    data = ( process_info_iter_data_t* )_data;

    if ( data->curr_index >= data->max_count ) {
        return 0;
    }

    info = ( process_info_t* )&data->info_table[ data->curr_index ];

    info->id = process->id;
    strncpy( info->name, process->name, MAX_PROCESS_NAME_LENGTH );
    info->name[ MAX_PROCESS_NAME_LENGTH - 1 ] = 0;
    info->pmem_size = process->pmem_size;
    info->vmem_size = process->vmem_size;

    data->curr_index++;

    return 0;
}

uint32_t sys_get_process_info( process_info_t* info_table, uint32_t max_count ) {
    process_info_iter_data_t data;

    data.curr_index = 0;
    data.max_count = max_count;
    data.info_table = info_table;

    spinlock_disable( &scheduler_lock );

    hashtable_iterate( &process_table, get_process_info_iterator, ( void* )&data );

    spinunlock_enable( &scheduler_lock );

    return data.curr_index;
}

process_id sys_getpid( void ) {
    return current_process()->id;
}

int sys_exit( int exit_code ) {
    process_t* process;

    process = current_process();

    /* Delete the waiters semaphore. This will release all
       waiter as well */

    delete_semaphore( process->waiters );
    process->waiters = -1;

    thread_exit( exit_code );

    return 0;
}

int sys_waitpid( process_id pid, int* status, int options ) {
    process_t* process;

    spinlock_disable( &scheduler_lock );

    process = get_process_by_id( pid );

    spinunlock_enable( &scheduler_lock );

    if ( process == NULL ) {
        return -EINVAL;
    }

    LOCK( process->waiters );

    return 0;
}

static void* process_key( hashitem_t* item ) {
    process_t* process;

    process = ( process_t* )item;

    return ( void* )process->id;
}

static uint32_t process_hash( const void* key ) {
    return ( uint32_t )key;
}

static bool process_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

int init_processes( void ) {
    int error;
    process_t* process;

    /* Initialize the process hashtable */

    error = init_hashtable(
        &process_table,
        128,
        process_key,
        process_hash,
        process_compare
    );

    if ( error < 0 ) {
        return error;
    }

    /* Create kernel process */

    process = allocate_process( "kernel" );

    if ( process == NULL ) {
        return -ENOMEM;
    }

    process->memory_context = &kernel_memory_context;
    process->semaphore_context = &kernel_semaphore_context;
    process->io_context = &kernel_io_context;

    /* TODO: This vmem setting is highly architecture dependent! */
    process->vmem_size = 512 * 1024 * 1024;

    kernel_memory_context.process = process;

    spinlock_disable( &scheduler_lock );
    error = insert_process( process );
    spinunlock_enable( &scheduler_lock );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
