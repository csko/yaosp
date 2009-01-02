/* Process implementation
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

#include <process.h>
#include <errno.h>
#include <semaphore.h>
#include <mm/kmalloc.h>
#include <mm/context.h>
#include <vfs/vfs.h>
#include <lib/hashtable.h>
#include <lib/string.h>

static int process_id_counter = 0;
static hashtable_t process_table;

process_t* allocate_process( char* name ) {
    process_t* process;

    process = ( process_t* )kmalloc( sizeof( process_t ) );

    if ( process == NULL ) {
        return NULL;
    }

    memset( process, 0, sizeof( process_t ) );

    process->id = -1;
    process->name = strdup( name );

    if ( process->name == NULL ) {
        kfree( process );
        return NULL;
    }

    return process;
}

int insert_process( process_t* process ) {
    do {
        process->id = process_id_counter++;

        if ( process_id_counter < 0 ) {
            process_id_counter = 0;
        }
    } while ( hashtable_get( &process_table, ( const void* )process->id ) != NULL );

    hashtable_add( &process_table, ( hashitem_t* )process );

    return 0;
}

process_t* get_process_by_id( process_id id ) {
    return ( process_t* )hashtable_get( &process_table, ( const void* )id );
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
                256,
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

    error = insert_process( process );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
