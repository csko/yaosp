/* Virtual file system
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
#include <macros.h>
#include <kernel.h>
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
    int max_free_inodes,
    uint32_t flags
) {
    int error;
    mount_point_t* mount_point;

    /* Allocate the memory for the mount point */

    mount_point = ( mount_point_t* )kmalloc( sizeof( mount_point_t ) );

    if ( mount_point == NULL ) {
        goto error1;
    }

    /* Initialize the inode cache */

    error = init_inode_cache( &mount_point->inode_cache, inode_cache_size, free_inodes, max_free_inodes );

    if ( error < 0 ) {
        goto error2;
    }

    /* Initialize other members */

    mount_point->fs_calls = fs_calls;
    mount_point->flags = flags;

    return mount_point;

error2:
    kfree( mount_point );

error1:
    return NULL;
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

static int do_read_stat( inode_t* inode, struct stat* stat );

int follow_symbolic_link( io_context_t* io_context, inode_t** _parent, inode_t** _inode ) {
    int error;
    struct stat st;
    inode_t* inode;
    char buffer[ 256 ];

    inode = *_inode;

    error = do_read_stat( inode, &st );

    if ( error < 0 ) {
        return error;
    }

    while ( st.st_mode & S_IFLNK ) {
        char* name;
        int name_length;
        inode_t* new_parent;
        inode_t* new_inode;

        if ( inode->mount_point->fs_calls->readlink == NULL ) {
            error = -ENOSYS;
        } else {
            error = inode->mount_point->fs_calls->readlink(
                inode->mount_point->fs_data,
                inode->fs_node,
                buffer,
                sizeof( buffer )
            );
        }

        if ( error < 0 ) {
            break;
        }

        error = lookup_parent_inode( io_context, *_parent, buffer, &name, &name_length, &new_parent );

        if ( error < 0 ) {
            break;
        }

        if ( ( name_length == 0 ) ||
             ( ( name_length == 1 ) && ( name[ 0 ] == '.' ) ) ) {
            atomic_inc( &new_parent->ref_count );
            new_inode = new_parent;
        } else {
            error = do_lookup_inode( io_context, new_parent, name, name_length, true, &new_inode );

            if ( error < 0 ) {
                put_inode( new_parent );
                break;
            }
        }

        put_inode( *_parent );
        put_inode( inode );
        *_parent = new_parent;
        inode = new_inode;

        error = do_read_stat( inode, &st );

        if ( error < 0 ) {
            break;
        }
    }

    *_inode = inode;

    return error;
}

static int do_open_helper1( io_context_t* io_context, file_t* file, inode_t** _parent, char* name, int length, int flags ) {
    int error;
    inode_t* parent;

    parent = *_parent;

    if ( ( length == 0 ) ||
         ( ( length == 1 ) && ( name[ 0 ] == '.' ) ) ) {
        /* Check if the current filesystem supports opening */

        if ( parent->mount_point->fs_calls->open == NULL ) {
            return -ENOSYS;
        }

        /* Open the inode */

        error = parent->mount_point->fs_calls->open(
            parent->mount_point->fs_data,
            parent->fs_node,
            flags,
            &file->cookie
        );

        if ( error < 0 ) {
            return error;
        }

        file->inode = parent;

        atomic_inc( &parent->ref_count );
    } else {
        /* Lookup the child */

        error = do_lookup_inode( io_context, parent, name, length, true, &file->inode );

        if ( error < 0 ) {
            return error;
        }

        /* Follow possible symbolic link */

        error = follow_symbolic_link( io_context, _parent, &file->inode );

        if ( error < 0 ) {
            return error;
        }

        ASSERT( file->inode != NULL );

        /* Check if the current filesystem supports opening */

        if ( file->inode->mount_point->fs_calls->open == NULL ) {
            return -ENOSYS;
        }

        /* Open the inode */

        error = file->inode->mount_point->fs_calls->open(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            flags,
            &file->cookie
        );

        if ( error < 0 ) {
            return error;
        }
    }

    return 0;
}

static int do_open_helper2( file_t* file, inode_t* parent, char* name, int length, int flags, int perms ) {
    int error;
    ino_t inode_number;

    if ( parent->mount_point->fs_calls->create == NULL ) {
        return -ENOSYS;
    }

    /* Create a new inode */

    error = parent->mount_point->fs_calls->create(
        parent->mount_point->fs_data,
        parent->fs_node,
        name,
        length,
        flags,
        perms,
        &inode_number,
        &file->cookie
    );

    if ( error < 0 ) {
        return error;
    }

    /* Assign the newly created inode with the file */

    file->inode = get_inode( parent->mount_point, inode_number );

    if ( file->inode == NULL ) {
        return -ENOINO;
    }

    return 0;
}

static int do_open( bool kernel, const char* path, int flags, int perms ) {
    int error;
    char* name;
    int length;
    file_t* file;
    struct stat st;
    inode_t* parent;
    io_context_t* io_context;

    /* Decide which I/O context to use */

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    /* Lookup the parent of the inode we want to open */

    error = lookup_parent_inode( io_context, NULL, path, &name, &length, &parent );

    if ( error < 0 ) {
        return error;
    }

    /* Check if the filesystem if writable and we want to create a file*/

    /* TODO: check the values of O_RDWR, O_WRONLY */
    if ( flags & O_CREAT && parent->mount_point->flags & MOUNT_RO ) {
        put_inode ( parent);
        return -EROFS;
    }

    /* Create a new file */

    file = create_file();

    if ( file == NULL ) {
        put_inode( parent );
        return -ENOMEM;
    }

    /* Open the requested inode */

    error = do_open_helper1( io_context, file, &parent, name, length, flags );

    if ( ( error == -ENOENT ) && ( ( flags & O_CREAT ) != 0 ) ) {
        error = do_open_helper2( file, parent, name, length, flags, perms );
    }

    put_inode( parent );

    if ( error < 0 ) {
        delete_file( file );
        return error;
    }

    error = do_read_stat( file->inode, &st );

    if ( error < 0 ) {
        delete_file( file );
        return error;
    }

    if ( st.st_mode & S_IFDIR ) {
        file->type = TYPE_DIRECTORY;
    } else {
        file->type = TYPE_FILE;
    }

    file->flags = flags;

    /* Insert the new file to the I/O context */

    error = io_context_insert_file( io_context, file );

    if ( error < 0 ) {
        delete_file( file );
        return error;
    }

    return file->fd;
}

int open( const char* path, int flags ) {
    return do_open( true, path, flags, 0666 );
}

int sys_open( const char* path, int flags ) {
    return do_open( false, path, flags, 0666 );
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

    ASSERT( atomic_get( &file->ref_count ) >= 2 );

    io_context_put_file( io_context, file ); /* this is for io_contetx_get_file() */
    io_context_put_file( io_context, file ); /* this will close the file */

    return 0;
}

int close( int fd ) {
    return do_close( true, fd );
}

int sys_close( int fd ) {
    return do_close( false, fd );
}

int do_pread_helper( file_t* file, void* buffer, size_t count, off_t offset ) {
    if ( file->inode->mount_point->fs_calls->read == NULL ) {
        return -ENOSYS;
    }

    return file->inode->mount_point->fs_calls->read(
        file->inode->mount_point->fs_data,
        file->inode->fs_node,
        file->cookie,
        buffer,
        offset,
        count
    );
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
        return -EBADF;
    }

    error = do_pread_helper( file, buffer, count, offset );

    io_context_put_file( io_context, file );

    return error;
}

int pread( int fd, void* buffer, size_t count, off_t offset ) {
    return do_pread( true, fd, buffer, count, offset );
}

int sys_pread( int fd, void* buffer, size_t count, off_t offset ) {
    return do_pread( false, fd, buffer, count, offset );
}

static int do_read( bool kernel, int fd, void* buffer, size_t count ) {
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

    if ( file->type == TYPE_DIRECTORY ) {
        error = -EISDIR;
        goto out;
    }

    if ( file->inode->mount_point->fs_calls->read != NULL ) {
        error = file->inode->mount_point->fs_calls->read(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            file->cookie,
            buffer,
            file->position,
            count
        );
    } else {
        error = -ENOSYS;
    }

    if ( error > 0 ) {
        file->position += error;
    }

out:
    io_context_put_file( io_context, file );

    return error;
}

int sys_read( int fd, void* buffer, size_t count ) {
    return do_read( false, fd, buffer, count );
}

static int do_pwrite( bool kernel, int fd, const void* buffer, size_t count, off_t offset ) {
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

    if ( file->type == TYPE_DIRECTORY ) {
        error = -EISDIR;
        goto out;
    }

    if ( file->inode->mount_point->flags & MOUNT_RO ) {
        error = -EROFS;
    } else if ( file->inode->mount_point->fs_calls->write != NULL ) {
        error = file->inode->mount_point->fs_calls->write(
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

out:
    io_context_put_file( io_context, file );

    return error;
}

int pwrite( int fd, const void* buffer, size_t count, off_t offset ) {
    return do_pwrite( true, fd, buffer, count, offset );
}

int sys_pwrite( int fd, const void* buffer, size_t count, off_t offset ) {
    return do_pwrite( false, fd, buffer, count, offset );
}

static int do_write( bool kernel, int fd, const void* buffer, size_t count ) {
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

    if ( file->type == TYPE_DIRECTORY ) {
        error = -EISDIR;
        goto out;
    }

    /* Check if the filesystem is writable */

    if ( file->inode->mount_point->flags & MOUNT_RO ) {
        error = -EROFS;
    } else if ( file->inode->mount_point->fs_calls->write != NULL ) {
        error = file->inode->mount_point->fs_calls->write(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            file->cookie,
            buffer,
            file->position,
            count
        );
    } else {
        error = -ENOSYS;
    }

    if ( error > 0 ) {
        file->position += error;
    }

out:
    io_context_put_file( io_context, file );

    return error;
}

int sys_write( int fd, const void* buffer, size_t count ) {
    return do_write( false, fd, buffer, count );
}

static int do_ioctl( bool kernel, int fd, int command, void* buffer ) {
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

    if ( file->inode->mount_point->fs_calls->ioctl != NULL ) {
        error = file->inode->mount_point->fs_calls->ioctl(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            file->cookie,
            command,
            buffer,
            kernel
        );
    } else {
        error = -ENOSYS;
    }

    io_context_put_file( io_context, file );

    return error;
}

int ioctl( int fd, int command, void* buffer ) {
    return do_ioctl( true, fd, command, buffer );
}

int sys_ioctl( int fd, int command, void* buffer ) {
    return do_ioctl( false, fd, command, buffer );
}

static int do_getdents( bool kernel, int fd, dirent_t* entry, unsigned int size ) {
    int ret;
    int error;
    file_t* file;
    unsigned int count;
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

    if ( file->type != TYPE_DIRECTORY ) {
        error = -ENOTDIR;
        goto out;
    }

    if ( file->inode->mount_point->fs_calls->read_directory == NULL ) {
        error = -ENOSYS;
        goto out;
    }

    ret = 0;
    count = size / sizeof( dirent_t );

    while ( count > 0 ) {
        error = file->inode->mount_point->fs_calls->read_directory(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            file->cookie,
            entry
        );

        if ( error == 0 ) {
            break;
        }

        if ( ( strcmp( entry->name, ".." ) == 0 ) &&
             ( entry->inode_number == file->inode->mount_point->root_inode_number ) &&
             ( file->inode->mount_point->mount_inode != NULL ) ) {
            entry->inode_number = file->inode->mount_point->mount_inode->inode_number;
        }

        ret++;
        count--;
        entry++;
    }

    error = ret;

out:
    io_context_put_file( io_context, file );

    return error;
}

int getdents( int fd, dirent_t* entry, unsigned int count ) {
    return do_getdents( true, fd, entry, count );
}

int sys_getdents( int fd, dirent_t* entry, unsigned int count ) {
    return do_getdents( false, fd, entry, count );
}

static int do_rewinddir( bool kernel, int fd ) {
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

    if ( file->type != TYPE_DIRECTORY ) {
        error = -ENOTDIR;
        goto out;
    }

    if ( file->inode->mount_point->fs_calls->rewind_directory == NULL ) {
        error = -ENOSYS;
        goto out;
    }

    error = file->inode->mount_point->fs_calls->rewind_directory(
        file->inode->mount_point->fs_data,
        file->inode->fs_node,
        file->cookie
    );

out:
    io_context_put_file( io_context, file );

    return error;
}

int sys_rewinddir( int fd ) {
    return do_rewinddir( false, fd );
}

static int do_isatty( bool kernel, int fd ) {
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

    if ( file->inode->mount_point->fs_calls->isatty == NULL ) {
        error = 0;
    } else {
        error = file->inode->mount_point->fs_calls->isatty(
            file->inode->mount_point->fs_data,
            file->inode->fs_node
        );
    }

    io_context_put_file( io_context, file );

    return error;
}

int sys_isatty( int fd ) {
    return do_isatty( false, fd );
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

    error = lookup_parent_inode( io_context, NULL, path, &name, &length, &parent );

    if ( error < 0 ) {
        return error;
    }

    /* Check if the filesystem is writable */

    if ( parent->mount_point->flags & MOUNT_RO ) {
        error = -EROFS;
    } else  if ( parent->mount_point->fs_calls->mkdir != NULL ) {
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

    put_inode( parent );

    return error;
}

int mkdir( const char* path, int permissions ) {
    return do_mkdir( true, path, permissions );
}

int sys_mkdir( const char* path, int permissions ) {
    return do_mkdir( false, path, permissions );
}

static int do_rmdir( bool kernel, const char* path ) {
    int error;
    inode_t* parent;
    io_context_t* io_context;

    char* name;
    int name_length;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_parent_inode( io_context, NULL, path, &name, &name_length, &parent );

    if ( error < 0 ) {
        return error;
    }

    if ( name_length == 0 ) {
        return -EINVAL;
    }

    if ( parent->mount_point->fs_calls->rmdir == NULL ) {
        error = -ENOSYS;
    } else {
        error = parent->mount_point->fs_calls->rmdir(
            parent->mount_point->fs_data,
            parent->fs_node,
            name,
            name_length
        );
    }

    put_inode( parent );

    return error;
}

int sys_rmdir( const char* _path ) {
    int error;
    char* path;
    size_t length;

    path = strdup( _path );

    if ( path == NULL ) {
        return -ENOMEM;
    }

    length = strlen( path );

    if ( ( length > 0 ) && ( path[ length - 1 ] == '/' ) ) {
        path[ length - 1 ] = 0;
    }

    error = do_rmdir( false, path );

    kfree( path );

    return error;
}

static int do_unlink( bool kernel, const char* path ) {
    int error;
    inode_t* parent;
    io_context_t* io_context;

    char* name;
    int name_length;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_parent_inode( io_context, NULL, path, &name, &name_length, &parent );

    if ( error < 0 ) {
        return error;
    }

    if ( name_length == 0 ) {
        return -EINVAL;
    }

    if ( parent->mount_point->fs_calls->unlink == NULL ) {
        error = -ENOSYS;
    } else {
        error = parent->mount_point->fs_calls->unlink(
            parent->mount_point->fs_data,
            parent->fs_node,
            name,
            name_length
        );
    }

    put_inode( parent );

    return error;
}

int sys_unlink( const char* path ) {
    return do_unlink( false, path );
}

static int do_symlink( bool kernel, const char* src, const char* dest ) {
    int error;
    io_context_t* io_context;

    char* name;
    int name_length;
    inode_t* inode;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_parent_inode( io_context, NULL, src, &name, &name_length, &inode );

    if ( error < 0 ) {
        return error;
    }

    if ( inode->mount_point->flags & MOUNT_RO ) {
        error = -EROFS;
    } else if ( inode->mount_point->fs_calls->symlink == NULL ) {
        error = -ENOSYS;
    } else {
        error = inode->mount_point->fs_calls->symlink(
            inode->mount_point->fs_data,
            inode->fs_node,
            name,
            name_length,
            dest
        );
    }

    put_inode( inode );

    return error;
}

int symlink( const char* src, const char* dest ) {
    return do_symlink( true, src, dest );
}

int sys_symlink( const char* src, const char* dest ) {
    return do_symlink( false, src, dest );
}

static int do_fchdir( bool kernel, int fd ) {
    int error = 0;
    file_t* file;
    inode_t* tmp;
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

    if ( file->type != TYPE_DIRECTORY ) {
        error = -ENOTDIR;
        goto out;
    }

    LOCK( io_context->lock );

    tmp = io_context->current_directory;
    io_context->current_directory = file->inode;

    atomic_inc( &io_context->current_directory->ref_count );

    UNLOCK( io_context->lock );

    put_inode( tmp );

out:
    io_context_put_file( io_context, file );

    return error;
}

int sys_chdir( const char* path ) {
    int fd;
    int error;

    fd = sys_open( path, O_RDONLY );

    if ( fd < 0 ) {
        return fd;
    }

    error = sys_fchdir( fd );

    sys_close( fd );

    return error;
}

int sys_fchdir( int fd ) {
    return do_fchdir( false, fd );
}

static int do_read_stat( inode_t* inode, struct stat* stat ) {
    int error;

    memset( stat, 0, sizeof( struct stat ) );

    if ( inode->mount_point->fs_calls->read_stat == NULL ) {
        error = -ENOSYS;
    } else {
        error = inode->mount_point->fs_calls->read_stat(
            inode->mount_point->fs_data,
            inode->fs_node,
            stat
        );
    }

    stat->st_dev = ( dev_t )inode->mount_point;

    return error;
}

static int do_stat( bool kernel, const char* path, struct stat* stat ) {
    int error;
    inode_t* inode;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_inode( io_context, NULL, path, &inode, true );

    if ( error < 0 ) {
        return error;
    }

    error = do_read_stat( inode, stat );

    put_inode( inode );

    return error;
}

int sys_stat( const char* path, struct stat* stat ) {
    return do_stat( false, path, stat );
}

static int do_lstat( bool kernel, const char* path, struct stat* stat ) {
    int error;
    inode_t* inode;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_inode( io_context, NULL, path, &inode, false );

    if ( error < 0 ) {
        return error;
    }

    error = do_read_stat( inode, stat );

    put_inode( inode );

    return error;
}

int sys_lstat( const char* path, struct stat* stat ) {
    return do_lstat( false, path, stat );
}

static int do_fstat( bool kernel, int fd, struct stat* stat ) {
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

    memset( stat, 0, sizeof( struct stat ) );

    if ( file->inode->mount_point->fs_calls->read_stat == NULL ) {
        error = -ENOSYS;
    } else {
        error = file->inode->mount_point->fs_calls->read_stat(
            file->inode->mount_point->fs_data,
            file->inode->fs_node,
            stat
        );
    }

    io_context_put_file( io_context, file );

    return error;
}

int fstat( int fd, struct stat* stat ) {
    return do_fstat( true, fd, stat );
}

int sys_fstat( int fd, struct stat* stat ) {
    return do_fstat( false, fd, stat );
}

static int do_lseek( bool kernel, int fd, off_t offset, int whence, off_t* new_offset ) {
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

    error = 0;

    switch ( whence ) {
        case SEEK_SET :
            if ( offset < 0 ) {
                error = -EINVAL;
                goto out;
            }

            file->position = offset;

            break;

        case SEEK_CUR :
            if ( ( file->position + offset ) < 0 ) {
                error = -EINVAL;
                goto out;
            } else {
                file->position += offset;
            }

            break;

        case SEEK_END : {
            struct stat st;

            error = do_read_stat( file->inode, &st );

            if ( error < 0 ) {
                goto out;
            }

            if ( ( st.st_size + offset ) < 0 ) {
                error = -EINVAL;
                goto out;
            } else {
                file->position = st.st_size + offset;
            }

            break;
        }

        default :
            error = -EINVAL;
            break;
    }

    if ( error >= 0 ) {
        *new_offset = file->position;
    }

out:
    io_context_put_file( io_context, file );

    return error;
}

off_t lseek( int fd, off_t offset, int whence ) {
    int error;
    off_t new_offset;

    error = do_lseek( true, fd, offset, whence, &new_offset );

    if ( error < 0 ) {
        return ( off_t )-1;
    }

    return new_offset;
}

int sys_lseek( int fd, off_t* offset, int whence, off_t* new_offset ) {
    return do_lseek( false, fd, *offset, whence, new_offset );
}

static int do_fcntl( bool kernel, int fd, int cmd, int arg ) {
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

    switch ( cmd ) {
        case F_GETFL :
            error = file->flags;
            break;

        case F_SETFL :
            if ( file->inode->mount_point->fs_calls->set_flags != NULL ) {
                error = file->inode->mount_point->fs_calls->set_flags(
                    file->inode->mount_point->fs_data,
                    file->inode->fs_node,
                    file->cookie,
                    arg
                );

                if ( error >= 0 ) {
                    file->flags = arg;
                }
            } else {
                error = -ENOSYS;
            }

            break;

        default :
            error = -EINVAL;
            break;
    }

    io_context_put_file( io_context, file );

    return error;
}

int sys_fcntl( int fd, int cmd, int arg ) {
    return do_fcntl( false, fd, cmd, arg );
}

static int do_readlink( bool kernel, const char* path, char* buffer, size_t length ) {
    int error;
    inode_t* inode;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_inode( io_context, NULL, path, &inode, true );

    if ( error < 0 ) {
        return error;
    }

    if ( inode->mount_point->fs_calls->readlink == NULL ) {
        error = -ENOSYS;
    } else {
        error = inode->mount_point->fs_calls->readlink(
            inode->mount_point->fs_data,
            inode->fs_node,
            buffer,
            length
        );
    }

    put_inode( inode );

    return error;
}

int sys_readlink( const char* path, char* buffer, size_t length ) {
    return do_readlink( false, path, buffer, length );
}

int do_mount( bool kernel, const char* device, const char* dir, const char* filesystem, uint32_t mountflags ) {
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

    error = lookup_inode( io_context, NULL, dir, &dir_inode, true );

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
        32,
        mountflags
    );

    if ( mount_point == NULL ) {
        put_inode( dir_inode );
        /* TODO: put filesystem? */
        return -ENOMEM;
    }

    error = fs_desc->calls->mount(
        device,
        mountflags,
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

    mount_point->root_inode_number = dir_inode->mount->inode_number;
    mount_point->mount_inode = dir_inode;

    /* NOTE: We don't have to call put_inode() on dir_inode because
             the mount point will own the reference to it */

    error = insert_mount_point( mount_point );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int mount( const char* device, const char* dir, const char* filesystem, uint32_t mountflags ) {
    return do_mount( true, device, dir, filesystem, mountflags );
}

int sys_mount( const char* device, const char* dir, const char* filesystem, uint32_t mountflags ) {
    return do_mount( false, device, dir, filesystem, mountflags );
}

int do_select( bool kernel, int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval_t* timeout ) {
    int i;
    int error;
    io_context_t* io_context;

    int req_count;
    int ready_count;
    file_t** files;
    semaphore_id sync;
    select_request_t* requests;

    if ( count <= 0 ) {
        return -EINVAL;
    }

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    requests = ( select_request_t* )kmalloc( sizeof( select_request_t ) * count * 3 );

    if ( requests == NULL ) {
        return -ENOMEM;
    }

    files = ( file_t** )kmalloc( sizeof( file_t* ) * count );

    if ( files == NULL ) {
        kfree( requests );
        return -ENOMEM;
    }

    sync = create_semaphore( "select sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( sync < 0 ) {
        kfree( files );
        kfree( requests );
        return sync;
    }

    req_count = 0;

    for ( i = 0; i < count; i++ ) {
        bool present = false;

        if ( ( readfds != NULL ) && ( FD_ISSET( i, readfds ) ) ) {
            present = true;
            requests[ req_count ].sync = sync;
            requests[ req_count ].ready = false;
            requests[ req_count ].type = SELECT_READ;
            requests[ req_count ].fd = i;
            req_count++;
        }
        if ( ( writefds != NULL ) && ( FD_ISSET( i, writefds ) ) ) {
            present = true;
            requests[ req_count ].sync = sync;
            requests[ req_count ].ready = false;
            requests[ req_count ].type = SELECT_WRITE;
            requests[ req_count ].fd = i;
            req_count++;
        }
        if ( ( exceptfds != NULL ) && ( FD_ISSET( i, exceptfds ) ) ) {
            present = true;
            requests[ req_count ].sync = sync;
            requests[ req_count ].ready = false;
            requests[ req_count ].type = SELECT_EXCEPT;
            requests[ req_count ].fd = i;
            req_count++;
        }

        if ( !present ) {
            files[ i ] = NULL;
            continue;
        }

        files[ i ] = io_context_get_file( io_context, i );

        if ( files[ i ] == NULL ) {
            /* TODO: cleanup */
            return -EBADF;
        }
    }

    /* Add select requests */

    for ( i = 0; i < req_count; i++ ) {
        file_t* file;

        file = files[ requests[ i ].fd ];

        if ( file->inode->mount_point->fs_calls->add_select_request == NULL ) {
            switch ( ( int )requests[ i ].type ) {
                case SELECT_READ :
                case SELECT_WRITE :
                    requests[ i ].ready = true;
                    UNLOCK( requests[ i ].sync );
                    break;
            }

            error = 0;
        } else {
            error = file->inode->mount_point->fs_calls->add_select_request(
                file->inode->mount_point->fs_data,
                file->inode->fs_node,
                file->cookie,
                &requests[ i ]
            );
        }

        if ( error < 0 ) {
            /* TODO: cleanup */
            return error;
        }
    }

    if ( timeout == NULL ) {
        lock_semaphore( sync, 1, INFINITE_TIMEOUT );
    } else {
        uint64_t _timeout;

        _timeout = ( uint64_t )timeout->tv_sec * 1000000 + ( uint64_t )timeout->tv_usec;

        lock_semaphore( sync, 1, _timeout );
    }

    if ( readfds != NULL ) {
        FD_ZERO( readfds );
    }

    if ( writefds != NULL ) {
        FD_ZERO( writefds );
    }

    if ( exceptfds != NULL ) {
        FD_ZERO( exceptfds );
    }

    ready_count = 0;

    for ( i = 0; i < req_count; i++ ) {
        file_t* file;

        file = files[ requests[ i ].fd ];

        /* Remove the select request */

        if ( file->inode->mount_point->fs_calls->remove_select_request != NULL ) {
            file->inode->mount_point->fs_calls->remove_select_request(
                file->inode->mount_point->fs_data,
                file->inode->fs_node,
                file->cookie,
                &requests[ i ]
            );
        }

        /* Check if this request is ready */

        if ( requests[ i ].ready ) {
            ready_count++;

            switch ( ( int )requests[ i ].type ) {
                case SELECT_READ :
                    FD_SET( requests[ i ].fd, readfds );
                    break;

                case SELECT_WRITE :
                    FD_SET( requests[ i ].fd, writefds );
                    break;

                case SELECT_EXCEPT :
                    FD_SET( requests[ i ].fd, exceptfds );
                    break;
            }
        }
    }

    /* Put the file handles */

    for ( i = 0; i < count; i++ ) {
        if ( files[ i ] != NULL ) {
            io_context_put_file( io_context, files[ i ] );
        }
    }

    /* Free allocated resources */

    delete_semaphore( sync );

    kfree( files );
    kfree( requests );

    return ready_count;
}

int select( int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval_t* timeout ) {
    return do_select( true, count, readfds, writefds, exceptfds, timeout );
}

int sys_select( int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval_t* timeout ) {
    return do_select( false, count, readfds, writefds, exceptfds, timeout );
}

static int do_dup( bool kernel, int old_fd ) {
    int error;
    file_t* old_file;
    file_t* new_file;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    old_file = io_context_get_file( io_context, old_fd );

    if ( old_file == NULL ) {
        return -EBADF;
    }

    new_file = create_file();

    if ( new_file == NULL ) {
        io_context_put_file( io_context, old_file );
        return -ENOMEM;
    }

    new_file->type = old_file->type;
    new_file->inode = old_file->inode;
    new_file->cookie = old_file->cookie;

    atomic_inc( &new_file->inode->ref_count );

    io_context_put_file( io_context, old_file );

    error = io_context_insert_file( io_context, new_file );

    if ( error < 0 ) {
        delete_file( new_file );
        return error;
    }

    return new_file->fd;
}

int sys_dup( int old_fd ) {
    return do_dup( false, old_fd );
}

static int do_dup2( bool kernel, int old_fd, int new_fd ) {
    int error;
    file_t* old_file;
    file_t* new_file;
    io_context_t* io_context;

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    old_file = io_context_get_file( io_context, old_fd );

    if ( old_file == NULL ) {
        return -EBADF;
    }

    new_file = create_file();

    if ( new_file == NULL ) {
        io_context_put_file( io_context, old_file );
        return -ENOMEM;
    }

    new_file->type = old_file->type;
    new_file->inode = old_file->inode;
    new_file->cookie = old_file->cookie;

    atomic_inc( &new_file->inode->ref_count );

    io_context_put_file( io_context, old_file );

    error = io_context_insert_file_with_fd( io_context, new_file, new_fd );

    if ( error < 0 ) {
        delete_file( new_file );
        return error;
    }

    return 0;
}

int sys_dup2( int old_fd, int new_fd ) {
    return do_dup2( false, old_fd, new_fd );
}

static int do_utime( bool kernel, const char* path, const struct utimbuf* times) {
    int error;
    inode_t* inode;
    io_context_t* io_context;

    struct stat _stat = {
        .st_atime = times->actime,
        .st_ctime = times->modtime
    };

    if ( kernel ) {
        io_context = &kernel_io_context;
    } else {
        io_context = current_process()->io_context;
    }

    error = lookup_inode( io_context, NULL, path, &inode, true );

    if ( error < 0 ) {
        return error;
    }

    if ( inode->mount_point->fs_calls->write_stat == NULL ) {
        error = -ENOSYS;
    } else {
        error = inode->mount_point->fs_calls->write_stat(
            inode->mount_point->fs_data,
            inode->fs_node,
            &_stat,
            WSTAT_MTIME | WSTAT_CTIME
        );
    }

    put_inode( inode );

    return error;
}

int sys_utime( const char* filename, const struct utimbuf* times ){
    return do_utime(false, filename, times);
}

__init int init_vfs( void ) {
    int error;

    mount_points = NULL;

    /* Initialize the kernel I/O context */

    error = init_io_context( &kernel_io_context );

    if ( error < 0 ) {
        goto error1;
    }

    /* Initialize filesystem manager */

    error = init_filesystems();

    if ( error < 0 ) {
        goto error1;
    }

    /* Initialize the root filesystem */

    error = init_root_filesystem();

    if ( error < 0 ) {
        goto error1;
    }

    /* Initialize and mount the device filesystem */

    error = init_devfs();

    if ( error < 0 ) {
        goto error1;
    }

    error = mkdir( "/device", 0 );

    if ( error < 0 ) {
        goto error1;
    }

    error = do_mount( true, "", "/device", "devfs", MOUNT_NONE );

    if ( error < 0 ) {
        return error;
    }

    return 0;

error1:
    return error;
}
