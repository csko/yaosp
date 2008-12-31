/* Virtual file system
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

#include <errno.h>
#include <smp.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <vfs/rootfs.h>
#include <vfs/inode.h>
#include <lib/string.h>

io_context_t kernel_io_context;

mount_point_t* create_mount_point(
    filesystem_calls_t* fs_calls,
    void* fs_data,
    int inode_cache_size,
    int free_inodes,
    int max_free_inodes
) {
    int error;
    mount_point_t* mount_point;

    /* Allocate the memory for the mount point */

    mount_point = ( mount_point_t* )kmalloc( sizeof( mount_point_t ) );

    if ( mount_point == NULL ) {
        return NULL;
    }

    /* Initialize the inode cache */

    error = init_inode_cache( &mount_point->inode_cache, inode_cache_size, free_inodes, max_free_inodes );

    if ( error < 0 ) {
        kfree( mount_point );
        return NULL;
    }

    /* Initialize other members */

    mount_point->fs_calls = fs_calls;
    mount_point->fs_data = fs_data;

    return mount_point;
}

static int lookup_parent_inode( io_context_t* io_context, const char* path, char** name, int* length, inode_t** _parent ) {
    int error;
    char* sep;
    inode_t* parent;

    LOCK( io_context->lock );

    if ( path[ 0 ] == '/' ) {
        parent = io_context->root_directory;
        path++;
    } else {
        parent = io_context->current_directory;
    }

    if ( parent == NULL ) {
        UNLOCK( io_context->lock );

        return -EINVAL;
    }

    atomic_inc( &parent->ref_count );

    UNLOCK( io_context->lock );

    while ( true ) {
        int length;
        ino_t inode_number;
        inode_t* inode;

        sep = strchr( path, '/' );

        if ( sep == NULL ) {
            break;
        }

        length = sep - path;

        error = parent->mount_point->fs_calls->lookup_inode(
            parent->mount_point->fs_data,
            parent->fs_node,
            path,
            length,
            &inode_number
        );

        if ( error < 0 ) {    
            put_inode( parent );
            return error;
        }

        inode = get_inode( parent->mount_point, inode_number );

        put_inode( parent );

        if ( inode == NULL ) {
            return -1;
        }

        parent = inode;
        path = sep + 1;
    }

    *name = ( char* )path;
    *length = strlen( path );
    *_parent = parent;

    return 0;
}

static int do_open( bool kernel, const char* path, int flags ) {
    int error;
    char* name;
    int length;
    file_t* file;
    inode_t* parent;
    io_context_t* io_context;

    /* Decide which I/O context to use */

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    /* Lookup the parent of the inode we want to open */

    error = lookup_parent_inode( io_context, path, &name, &length, &parent );

    if ( error < 0 ) {
        return error;
    }

    /* Create a new file */

    file = create_file();

    if ( file == NULL ) {
        put_inode( parent );
        return -ENOMEM;
    }

    if ( ( length == 0 ) ||
         ( ( length == 1 ) && ( name[ 0 ] == '.' ) ) ) {
        error = parent->mount_point->fs_calls->open(
            parent->mount_point->fs_data,
            parent->fs_node,
            0 /* mode */,
            &file->cookie
        );

        if ( error < 0 ) {
            delete_file( file );
            put_inode( parent );
            return error;
        }

        file->inode = parent;
    } else {
        ino_t child_inode;

        error = parent->mount_point->fs_calls->lookup_inode(
            parent->mount_point->fs_data,
            parent->fs_node,
            name,
            length,
            &child_inode
        );

        put_inode( parent );

        if ( error < 0 ) {
            delete_file( file );
            return error;
        }

        file->inode = get_inode( parent->mount_point, child_inode );

        if ( file->inode == NULL ) {
            delete_file( file );
            return error;
        }

        error = file->inode->mount_point->fs_calls->open(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            0 /* mode */,
            &file->cookie
        );

        if ( error < 0 ) {
            put_inode( file->inode );
            delete_file( file );
            return error;
        }
    }

    error = io_context_insert_file( io_context, file );

    if ( error < 0 ) {
        put_inode( file->inode );
        delete_file( file );
        return error;
    }

    return file->fd;
}

int open( const char* path, int flags ) {
    return do_open( true, path, flags );
}

static int do_getdents( bool kernel, int fd, dirent_t* entry ) {
    int error;
    file_t* file;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    file = io_context_get_file( io_context, fd );

    if ( file == NULL ) {
        return -EINVAL; /* TODO: right error code? */
    }

    /* TODO: check file type */

    error = file->inode->mount_point->fs_calls->read_directory(
        file->inode->mount_point->fs_data,
        file->inode->fs_node,
        file->cookie,
        entry
    );

    return error;
}

int getdents( int fd, dirent_t* entry ) {
    return do_getdents( true, fd, entry );
}

int init_vfs( void ) {
    int error;

    /* Initialize the kernel I/O context */

    error = init_io_context( &kernel_io_context );

    if ( error < 0 ) {
        return error;
    }

    /* Initialize the root filesystem */

    error = init_root_filesystem();

    if ( error < 0 ) {
        /* TODO: free io context stuffs */
        return error;
    }

    return 0;
}
