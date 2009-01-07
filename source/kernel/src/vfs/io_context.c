/* I/O context
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

#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/io_context.h>
#include <lib/string.h>

file_t* create_file( void ) {
    file_t* file;

    file = ( file_t* )kmalloc( sizeof( file_t ) );

    if ( file == NULL ) {
        return NULL;
    }

    memset( file, 0, sizeof( file_t ) );

    atomic_set( &file->ref_count, 0 );

    return file;
}

void delete_file( file_t* file ) {
    if ( file->inode != NULL ) {
        put_inode( file->inode );
        file->inode = NULL;
    }

    kfree( file );
}

int io_context_insert_file( io_context_t* io_context, file_t* file ) {
    atomic_set( &file->ref_count, 1 );

    LOCK( io_context->lock );

    do {
        file->fd = io_context->fd_counter++;

        if ( io_context->fd_counter < 0 ) {
            io_context->fd_counter = 3;
        }
    } while ( hashtable_get( &io_context->file_table, ( const void* )file->fd ) != NULL );

    hashtable_add( &io_context->file_table, ( hashitem_t* )file );

    UNLOCK( io_context->lock );

    return 0;
}

file_t* io_context_get_file( io_context_t* io_context, int fd ) {
    file_t* file;

    LOCK( io_context->lock );

    file = ( file_t* )hashtable_get( &io_context->file_table, ( const void* )fd );

    UNLOCK( io_context->lock );

    if ( file != NULL ) {
        atomic_inc( &file->ref_count );
    }

    return file;
}

void io_context_put_file( io_context_t* io_context, file_t* file ) {
    ASSERT( file != NULL );

    if ( atomic_dec_and_test( &file->ref_count ) ) {
        LOCK( io_context->lock );

        hashtable_remove( &io_context->file_table, ( const void* )file->fd );

        UNLOCK( io_context->lock );

        delete_file( file );
    }
}

io_context_t* io_context_clone( io_context_t* old_io_context ) {
    int error;
    io_context_t* new_io_context;

    new_io_context = ( io_context_t* )kmalloc( sizeof( io_context_t ) );

    if ( new_io_context == NULL ) {
        return NULL;
    }

    error = init_io_context( new_io_context );

    if ( error < 0 ) {
        kfree( new_io_context );
        return NULL;
    }

    /* TODO: clone stuffs */

    return new_io_context;
}

static void* file_key( hashitem_t* item ) {
    file_t* file;

    file = ( file_t* )item;

    return ( void* )file->fd;
}

static uint32_t file_hash( const void* key ) {
    return hash_number( ( uint8_t* )&key, sizeof( int ) );
}

static bool file_compare( const void* key1, const void* key2 ) {
    return key1 == key2;
}

int init_io_context( io_context_t* io_context ) {
    int error;

    /* Initialize the file table */

    error = init_hashtable(
        &io_context->file_table,
        32,
        file_key,
        file_hash,
        file_compare
    );

    io_context->fd_counter = 3;

    /* Create the I/O context lock */

    io_context->lock = create_semaphore( "I/O context lock", SEMAPHORE_BINARY, 0, 1 );

    if ( io_context->lock < 0 ) {
        return io_context->lock;
    }

    return 0;
}
