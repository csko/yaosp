/* Virtual file system
 *
 * Copyright (c) 2008 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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
#include <vfs/devfs.h>
#include <lib/string.h>

io_context_t kernel_io_context;
mount_point_t* mount_points;

mount_point_t* create_mount_point(
    filesystem_calls_t* fs_calls,
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

    return mount_point;
}

void delete_mount_point( mount_point_t* mount_point ) {
    destroy_inode_cache( &mount_point->inode_cache );
    kfree( mount_point );
}

int insert_mount_point( mount_point_t* mount_point ) {
    mount_point->next = mount_points;
    mount_points = mount_point;

    return 0;
}

static int do_open_helper1( file_t* file, inode_t* parent, char* name, int length ) {
    int error;

    if ( ( length == 0 ) ||
         ( ( length == 1 ) && ( name[ 0 ] == '.' ) ) ) {
        error = parent->mount_point->fs_calls->open(
            parent->mount_point->fs_data,
            parent->fs_node,
            0 /* mode */,
            &file->cookie
        );

        if ( error < 0 ) {
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

        if ( error < 0 ) {
            return error;
        }

        file->inode = get_inode( parent->mount_point, child_inode );

        if ( file->inode == NULL ) {
            return -ENOINO;
        }

        /* Follow the mount point */

        if ( file->inode->mount != NULL ) {
            inode_t* tmp;

            tmp = file->inode;
            file->inode = tmp->mount;

            atomic_inc( &file->inode->ref_count );
            put_inode( tmp );
        }

        error = file->inode->mount_point->fs_calls->open(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            0 /* mode */,
            &file->cookie
        );

        if ( error < 0 ) {
            return error;
        }
    }

    return 0;
}

static int do_open_helper2( file_t* file, inode_t* parent, char* name, int length ) {
    int error;
    ino_t inode_number;

    if ( parent->mount_point->fs_calls->create == NULL ) {
        return -ENOSYS;
    }

    error = parent->mount_point->fs_calls->create(
        parent->mount_point->fs_data,
        parent->fs_node,
        name,
        length,
        0,
        0,
        &inode_number,
        &file->cookie
    );

    if ( error < 0 ) {
        return error;
    }

    file->inode = get_inode( parent->mount_point, inode_number );

    if ( file->inode == NULL ) {
        return -ENOINO;
    }

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

    error = do_open_helper1( file, parent, name, length );

    if ( ( error == -ENOENT ) && ( ( flags & O_CREAT ) != 0 ) ) {
        error = do_open_helper2( file, parent, name, length );
    }

    put_inode( parent );

    if ( error < 0 ) {
        delete_file( file );
        return error;
    }

    error = io_context_insert_file( io_context, file );

    if ( error < 0 ) {
        delete_file( file );
        return error;
    }

    return file->fd;
}

int open( const char* path, int flags ) {
    return do_open( true, path, flags );
}

static int do_close( bool kernel, int fd ) {
    file_t* file;
    io_context_t* io_context;

    /* Decide which I/O context to use */

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    file = io_context_get_file( io_context, fd );

    if ( file == NULL ) {
        return -EBADF;
    }

    io_context_put_file( io_context, file ); /* this is for io_contetx_get_file() */
    io_context_put_file( io_context, file ); /* this will close the file */

    return 0;
}

int close( int fd ) {
    return do_close( true, fd );
}

static int do_pread( bool kernel, int fd, void* buffer, size_t count, off_t offset ) {
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

    if ( file->inode->mount_point->fs_calls->read != NULL ) {
        error = file->inode->mount_point->fs_calls->read(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            file->cookie,
            buffer,
            offset,
            count
        );
    } else {
        error = -ENOSYS;
    }

    io_context_put_file( io_context, file );

    return error;
}

int pread( int fd, void* buffer, size_t count, off_t offset ) {
    return do_pread( true, fd, buffer, count, offset );
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
        return -EBADF;
    }

    /* TODO: check file type */

    if ( file->inode->mount_point->fs_calls->read_directory == NULL ) {
        error = -ENOSYS;
    } else {
        error = file->inode->mount_point->fs_calls->read_directory(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            file->cookie,
            entry
        );
    }

    io_context_put_file( io_context, file );

    return error;
}

int getdents( int fd, dirent_t* entry ) {
    return do_getdents( true, fd, entry );
}

static int do_mkdir( bool kernel, const char* path, int permissions ) {
    int error;
    char* name;
    int length;
    inode_t* parent;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_parent_inode( io_context, path, &name, &length, &parent );

    if ( error < 0 ) {
        return error;
    }

    if ( parent->mount_point->fs_calls->mkdir != NULL ) {
        error = parent->mount_point->fs_calls->mkdir(
            parent->mount_point->fs_data,
            parent->fs_node,
            name,
            length,
            permissions
        );
    } else {
        error = -ENOSYS;
    }

    return error;
}

int mkdir( const char* path, int permissions ) {
    return do_mkdir( true, path, permissions );
}

int do_mount( bool kernel, const char* device, const char* dir, const char* filesystem ) {
    int error;
    inode_t* dir_inode;
    io_context_t* io_context;
    filesystem_descriptor_t* fs_desc;
    mount_point_t* mount_point;
    ino_t root_inode_number;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_inode( io_context, dir, &dir_inode );

    if ( error < 0 ) {
        return error;
    }

    if ( dir_inode->mount != NULL ) {
        put_inode( dir_inode );
        return -EBUSY;
    }

    fs_desc = get_filesystem( filesystem );

    if ( fs_desc == NULL ) {
        put_inode( dir_inode );
        return -EINVAL;
    }

    mount_point = create_mount_point(
        fs_desc->calls,
        256,
        16,
        32
    );

    if ( mount_point == NULL ) {
        put_inode( dir_inode );
        /* TODO: put filesystem? */
        return -ENOMEM;
    }

    error = fs_desc->calls->mount(
        device,
        0,
        &mount_point->fs_data,
        &root_inode_number
    );

    if ( error < 0 ) {
        put_inode( dir_inode );
        /* TODO: put filesystem? */
        /* TODO: free mount point */
        return error;
    }

    dir_inode->mount = get_inode( mount_point, root_inode_number );

    if ( dir_inode->mount == NULL ) {
        put_inode( dir_inode );
        /* TODO: cleanup! :D */
        return -ENOMEM;
    }

    put_inode( dir_inode );

    error = insert_mount_point( mount_point );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int mount( const char* device, const char* dir, const char* filesystem ) {
    return do_mount( true, device, dir, filesystem );
}

int init_vfs( void ) {
    int error;

    mount_points = NULL;

    /* Initialize the kernel I/O context */

    error = init_io_context( &kernel_io_context );

    if ( error < 0 ) {
        return error;
    }

    /* Initialize filesystem manager */

    error = init_filesystems();

    if ( error < 0 ) {
        /* TODO: cleanup */
        return error;
    }

    /* Initialize the root filesystem */

    error = init_root_filesystem();

    if ( error < 0 ) {
        /* TODO: free io context stuffs */
        return error;
    }

    /* Initialize and mount the device filesystem */

    error = init_devfs();

    if ( error < 0 ) {
        /* TODO: cleanup */
        return error;
    }

    error = mkdir( "/device", 0 );

    if ( error < 0 ) {
        /* TODO: cleanup */
        return error;
    }

    error = do_mount( true, "", "/device", "devfs" );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
