/* I/O context handling
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

#include <macros.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <vfs/io_context.h>
#include <vfs/vfs.h>
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

int io_context_insert_file( io_context_t* io_context, file_t* file, int start_fd ) {
    size_t i;
    int error;

    LOCK( io_context->lock );

    for ( i = start_fd; i < io_context->file_table_size; i++ ) {
        if ( io_context->file_table[ i ] == NULL ) {
            break;
        }
    }

    if ( i == io_context->file_table_size ) {
        error = -ENOMEM;
        goto out;
    }

    io_context->file_table[ i ] = file;

    atomic_inc( &file->ref_count );

    error = ( int )i;

out:
    UNLOCK( io_context->lock );

    return error;
}

int io_context_insert_file_at( io_context_t* io_context, file_t* file, int fd, bool close_existing ) {
    int error;

    atomic_inc( &file->ref_count );

    LOCK( io_context->lock );

    if ( ( fd < 0 ) || ( fd >= io_context->file_table_size ) ) {
        error = -EINVAL;
        goto out;
    }

    if ( io_context->file_table[ fd ] != NULL ) {
        if ( close_existing ) {
            io_context_put_file( io_context, io_context->file_table[ fd ] );
        } else {
            error = -EEXIST;
            goto out;
        }
    }

    io_context->file_table[ fd ] = file;

    error = 0;

out:
    UNLOCK( io_context->lock );

    return error;
}

int io_context_remove_file( io_context_t* io_context, int fd ) {
    int error;
    file_t* file;

    file = NULL;

    LOCK( io_context->lock );

    if ( ( fd < 0 ) || ( fd >= io_context->file_table_size ) ) {
        error = -EBADF;
        goto out;
    }

    file = io_context->file_table[ fd ];

    if ( file == NULL ) {
        error = -EBADF;
        goto out;
    }

    io_context->file_table[ fd ] = NULL;

    error = 0;

out:
    UNLOCK( io_context->lock );

    if ( file != NULL ) {
        io_context_put_file( io_context, file );
    }

    return error;
}

file_t* io_context_get_file( io_context_t* io_context, int fd ) {
    file_t* file;

    LOCK( io_context->lock );

    if ( ( fd < 0 ) || ( fd >= io_context->file_table_size ) ) {
        file = NULL;
        goto out;
    }

    file = io_context->file_table[ fd ];

    if ( file == NULL ) {
        goto out;
    }

    atomic_inc( &file->ref_count );

 out:
    UNLOCK( io_context->lock );

    return file;
}

void io_context_put_file( io_context_t* io_context, file_t* file ) {
    inode_t* inode;

    if ( !atomic_dec_and_test( &file->ref_count ) ) {
        return;
    }


    ASSERT( file->inode != NULL );

    inode = file->inode;

    /* Close the file  */

    if ( inode->mount_point->fs_calls->close != NULL ) {
        inode->mount_point->fs_calls->close(
            inode->mount_point->fs_data,
            inode->fs_node,
            file->cookie
        );
    }

    /* Free the cookie */

    if ( inode->mount_point->fs_calls->free_cookie != NULL ) {
        inode->mount_point->fs_calls->free_cookie(
            inode->mount_point->fs_data,
            inode->fs_node,
            file->cookie
        );
    }

    /* Delete the file */

    delete_file( file );
}

io_context_t* io_context_clone( io_context_t* old_io_context ) {
    int error;
    size_t i;
    io_context_t* new_io_context;

    new_io_context = ( io_context_t* )kmalloc( sizeof( io_context_t ) );

    if ( new_io_context == NULL ) {
        return NULL;
    }

    error = init_io_context( new_io_context, old_io_context->file_table_size );

    if ( error < 0 ) {
        kfree( new_io_context );
        return NULL;
    }

    LOCK( old_io_context->lock );

    /* Clone root and current working directory */

    new_io_context->root_directory = old_io_context->root_directory;
    atomic_inc( &new_io_context->root_directory->ref_count );

    new_io_context->current_directory = old_io_context->current_directory;
    atomic_inc( &new_io_context->current_directory->ref_count );

    /* Clone file handles */

    memcpy( new_io_context->file_table, old_io_context->file_table, sizeof( file_t* ) * old_io_context->file_table_size );

    for ( i = 0; i < new_io_context->file_table_size; i++ ) {
        if ( new_io_context->file_table[ i ] == NULL ) {
            continue;
        }

        atomic_inc( &new_io_context->file_table[ i ]->ref_count );
    }

    UNLOCK( old_io_context->lock );

    return new_io_context;
}

int init_io_context( io_context_t* io_context, size_t file_table_size ) {
    int error;

    /* Create the I/O context lock */

    io_context->lock = create_semaphore( "I/O context lock", SEMAPHORE_BINARY, 0, 1 );

    if ( io_context->lock < 0 ) {
        error = io_context->lock;
        goto error1;
    }

    /* Create the file table */

    io_context->file_table = ( file_t** )kmalloc( sizeof( file_t* ) * file_table_size );

    if ( io_context->file_table == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    memset( io_context->file_table, 0, sizeof( file_t* ) * file_table_size );

    io_context->file_table_size = file_table_size;

    return 0;

error2:
    delete_semaphore( io_context->lock );

error1:
    return error;
}

void destroy_io_context( io_context_t* io_context ) {
    size_t i;

    /* Delete all files and the file table */

    for ( i = 0; i < io_context->file_table_size; i++ ) {
        if ( io_context->file_table[ i ] == NULL ) {
            continue;
        }

        io_context_put_file( io_context, io_context->file_table[ i ] );
    }

    kfree( io_context->file_table );

    /* Put the inodes in the I/O context */

    put_inode( io_context->root_directory );
    put_inode( io_context->current_directory );

    /* Delete the lock of the context */

    delete_semaphore( io_context->lock );
    kfree( io_context );
}
