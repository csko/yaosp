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
#include <console.h>
#include <mm/kmalloc.h>
#include <lock/semaphore.h>
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

int remove_mount_point( mount_point_t* mount_point ) {
    mount_point_t* prev;
    mount_point_t* current;

    prev = NULL;
    current = mount_points;

    while ( current != NULL ) {
        if ( current == mount_point ) {
            if ( prev == NULL ) {
                mount_points = current->next;
            } else {
                prev->next = current->next;
            }

            break;
        }

        prev = current;
        current = current->next;
    }

    return 0;
}

mount_point_t* get_mount_point_by_cookie( void* cookie ) {
    mount_point_t* current;

    current = mount_points;

    while ( current != NULL ) {
        if ( current->fs_data == cookie ) {
            return current;
        }

        current = current->next;
    }

    return NULL;
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

    while ( ( st.st_mode & S_IFMT ) == S_IFLNK ) {
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

static int do_open( io_context_t* io_context, const char* path, int flags, int perms ) {
    int error;
    char* name;
    int length;
    file_t* file;
    struct stat st;
    inode_t* parent;

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

    if ( ( st.st_mode & S_IFMT ) == S_IFDIR ) {
        file->type = TYPE_DIRECTORY;
    } else {
        file->type = TYPE_FILE;
    }

    file->flags = flags;

    /* Insert the new file to the I/O context */

    error = io_context_insert_file( io_context, file, 3 );

    if ( error < 0 ) {
        delete_file( file );
    }

    return error;
}

int open( const char* path, int flags ) {
    return do_open( &kernel_io_context, path, flags, 0666 );
}

int sys_open( const char* path, int flags ) {
    return do_open( current_process()->io_context, path, flags, 0666 );
}

static int do_close( io_context_t* io_context, int fd ) {
    int error;

    error = io_context_remove_file( io_context, fd );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int close( int fd ) {
    return do_close( &kernel_io_context, fd );
}

int sys_close( int fd ) {
    return do_close( current_process()->io_context, fd );
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

static int do_pread( io_context_t* io_context, int fd, void* buffer, size_t count, off_t offset ) {
    int error;
    file_t* file;

    file = io_context_get_file( io_context, fd );

    if ( file == NULL ) {
        return -EBADF;
    }

    error = do_pread_helper( file, buffer, count, offset );

    io_context_put_file( io_context, file );

    return error;
}

int pread( int fd, void* buffer, size_t count, off_t offset ) {
    return do_pread( &kernel_io_context, fd, buffer, count, offset );
}

int sys_pread( int fd, void* buffer, size_t count, off_t* offset ) {
    if ( offset == NULL ) {
        return -EINVAL;
    }

    return do_pread( current_process()->io_context, fd, buffer, count, *offset );
}

static int do_read( io_context_t* io_context, int fd, void* buffer, size_t count ) {
    int error;
    file_t* file;

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
    return do_read( current_process()->io_context, fd, buffer, count );
}

static int do_pwrite( io_context_t* io_context, int fd, const void* buffer, size_t count, off_t offset ) {
    int error;
    file_t* file;

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
    return do_pwrite( &kernel_io_context, fd, buffer, count, offset );
}

int sys_pwrite( int fd, const void* buffer, size_t count, off_t* offset ) {
    return do_pwrite( current_process()->io_context, fd, buffer, count, *offset );
}

static int do_write( io_context_t* io_context, int fd, const void* buffer, size_t count ) {
    int error;
    file_t* file;

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
    return do_write( current_process()->io_context, fd, buffer, count );
}

static int do_ioctl( io_context_t* io_context, int fd, int command, void* buffer ) {
    int error;
    file_t* file;

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
            io_context == &kernel_io_context
        );
    } else {
        error = -ENOSYS;
    }

    io_context_put_file( io_context, file );

    return error;
}

int ioctl( int fd, int command, void* buffer ) {
    return do_ioctl( &kernel_io_context, fd, command, buffer );
}

int sys_ioctl( int fd, int command, void* buffer ) {
    return do_ioctl( current_process()->io_context, fd, command, buffer );
}

static int do_getdents( io_context_t* io_context, int fd, dirent_t* entry, unsigned int size ) {
    int ret;
    int error;
    file_t* file;
    unsigned int count;

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
    return do_getdents( &kernel_io_context, fd, entry, count );
}

int sys_getdents( int fd, dirent_t* entry, unsigned int count ) {
    return do_getdents( current_process()->io_context, fd, entry, count );
}

static int do_rewinddir( io_context_t* io_context, int fd ) {
    int error;
    file_t* file;

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
    return do_rewinddir( current_process()->io_context, fd );
}

static int do_isatty( io_context_t* io_context, int fd ) {
    int error;
    file_t* file;

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
    return do_isatty( current_process()->io_context, fd );
}

static int do_access( io_context_t* io_context, const char* path, int mode ) {
    int error;
    inode_t* inode;

    error = lookup_inode( io_context, NULL, path, &inode, true, true );

    if ( error < 0 ) {
        return error;
    }

    put_inode( inode );

    return 0;
}

int sys_access( const char* path, int mode ) {
    return do_access( current_process()->io_context, path, mode );
}

static int do_mkdir( io_context_t* io_context, const char* path, int permissions ) {
    int error;
    char* name;
    int length;
    inode_t* parent;

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
    return do_mkdir( &kernel_io_context, path, permissions );
}

int sys_mkdir( const char* path, int permissions ) {
    return do_mkdir( current_process()->io_context, path, permissions );
}

static int do_rmdir( io_context_t* io_context, const char* path ) {
    int error;
    char* name;
    inode_t* parent;
    int name_length;

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

    error = do_rmdir( current_process()->io_context, path );

    kfree( path );

    return error;
}

static int do_unlink( io_context_t* io_context, const char* path ) {
    int error;
    char* name;
    inode_t* parent;
    int name_length;

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
    return do_unlink( current_process()->io_context, path );
}

static int do_symlink( io_context_t* io_context, const char* src, const char* dest ) {
    int error;
    char* name;
    inode_t* inode;
    int name_length;

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
    return do_symlink( &kernel_io_context, src, dest );
}

int sys_symlink( const char* src, const char* dest ) {
    return do_symlink( current_process()->io_context, src, dest );
}

static int do_fchdir( io_context_t* io_context, int fd ) {
    int error = 0;
    file_t* file;
    inode_t* tmp;

    file = io_context_get_file( io_context, fd );

    if ( file == NULL ) {
        return -EBADF;
    }

    if ( file->type != TYPE_DIRECTORY ) {
        error = -ENOTDIR;
        goto out;
    }

    mutex_lock( io_context->mutex, LOCK_IGNORE_SIGNAL );

    tmp = io_context->current_directory;
    io_context->current_directory = file->inode;

    atomic_inc( &io_context->current_directory->ref_count );

    mutex_unlock( io_context->mutex );

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
    return do_fchdir( current_process()->io_context, fd );
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

static int do_stat( io_context_t* io_context, const char* path, struct stat* stat, bool follow_symlink ) {
    int error;
    inode_t* inode;

    error = lookup_inode( io_context, NULL, path, &inode, follow_symlink, true );

    if ( error < 0 ) {
        return error;
    }

    error = do_read_stat( inode, stat );

    put_inode( inode );

    return error;
}

int sys_stat( const char* path, struct stat* stat ) {
    return do_stat( current_process()->io_context, path, stat, true );
}

int sys_lstat( const char* path, struct stat* stat ) {
    return do_stat( current_process()->io_context, path, stat, false );
}

static int do_fstat( io_context_t* io_context, int fd, struct stat* stat ) {
    int error;
    file_t* file;

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
    return do_fstat( &kernel_io_context, fd, stat );
}

int sys_fstat( int fd, struct stat* stat ) {
    return do_fstat( current_process()->io_context, fd, stat );
}

static int do_lseek( io_context_t* io_context, int fd, off_t offset, int whence, off_t* new_offset ) {
    int error;
    file_t* file;

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

    error = do_lseek( &kernel_io_context, fd, offset, whence, &new_offset );

    if ( error < 0 ) {
        return ( off_t )-1;
    }

    return new_offset;
}

int sys_lseek( int fd, off_t* offset, int whence, off_t* new_offset ) {
    return do_lseek( current_process()->io_context, fd, *offset, whence, new_offset );
}

static int do_fcntl( io_context_t* io_context, int fd, int cmd, int arg ) {
    int error;
    file_t* file;

    file = io_context_get_file( io_context, fd );

    if ( file == NULL ) {
        return -EBADF;
    }

    switch ( cmd ) {
        case F_DUPFD :
            error = io_context_insert_file( io_context, file, 3 );
            break;

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

        case F_GETFD :
            error = ( file->close_on_exec ? 0x01 : 0x00 );

            break;

        case F_SETFD :
            if ( arg & 0x01 ) {
                file->close_on_exec = true;
            } else {
                file->close_on_exec = false;
            }

            error = 0;

            break;

        default :
            kprintf( WARNING, "do_fcntl(): Unhandled fcntl command: %x\n", cmd );
            error = -EINVAL;
            break;
    }

    io_context_put_file( io_context, file );

    return error;
}

int sys_fcntl( int fd, int cmd, int arg ) {
    return do_fcntl( current_process()->io_context, fd, cmd, arg );
}

static int do_readlink( io_context_t* io_context, const char* path, char* buffer, size_t length ) {
    int error;
    inode_t* inode;

    error = lookup_inode( io_context, NULL, path, &inode, false, true );

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
    return do_readlink( current_process()->io_context, path, buffer, length );
}

static int do_mount( io_context_t* io_context, const char* device,
                     const char* dir, const char* filesystem, uint32_t mountflags ) {
    int error;
    inode_t* dir_inode;
    filesystem_descriptor_t* fs_desc;
    mount_point_t* mount_point;
    ino_t root_inode_number;

    error = lookup_inode( io_context, NULL, dir, &dir_inode, true, false );

    if ( error < 0 ) {
        return error;
    }

    if ( dir_inode->mount != NULL ) {
        put_inode( dir_inode );
        return -EBUSY;
    }

    if ( filesystem != NULL ) {
        fs_desc = get_filesystem( filesystem );
    } else {
        fs_desc = probe_filesystem( device );
    }

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
    return do_mount( &kernel_io_context, device, dir, filesystem, mountflags );
}

int sys_mount( const char* device, const char* dir, const char* filesystem, uint32_t mountflags ) {
    return do_mount( current_process()->io_context, device, dir, filesystem, mountflags );
}

static int do_unmount( io_context_t* io_context, const char* dir ) {
    int error;
    inode_t* inode;
    inode_t* mount_root;
    mount_point_t* mount_point;

    error = lookup_inode( io_context, NULL, dir, &inode, true, false );

    if ( error < 0 ) {
        goto error1;
    }

    if ( inode->mount == NULL ) {
        error = -EINVAL;
        goto error2;
    }

    /* Make sure only the root inode is in the cache, otherwise
       we can't unmount the filesystem because it's busy! */

    mount_point = inode->mount->mount_point;

    if ( mount_point_can_unmount( mount_point ) != 0 ) {
        return -EBUSY;
    }

    /* Release the last reference to the root inode */

    mount_root = inode->mount;
    inode->mount = NULL;

    put_inode( mount_root );

    /* The inode cache of the mount point should be empty now ... */

    ASSERT( get_inode_cache_size( &mount_point->inode_cache ) == 0 );

    /* Remove the mount point */

    remove_mount_point( mount_point );

    /* Call the unmount on the filesystem */

    if ( mount_point->fs_calls->unmount != NULL ) {
        mount_point->fs_calls->unmount(
            mount_point->fs_data
        );
    }

    /* Delete the mount point */

    delete_mount_point( mount_point );

    return 0;

error2:
    put_inode( inode );

error1:
    return error;
}

int sys_unmount( const char* dir ) {
    return do_unmount( current_process()->io_context, dir );
}

int do_select( io_context_t* io_context, int count, fd_set* readfds,
               fd_set* writefds, fd_set* exceptfds, timeval_t* timeout ) {
    int i;
    int error;
    lock_id sync;
    int req_count;
    int ready_count;
    file_t** files;
    select_request_t* requests;

    if ( count <= 0 ) {
        return -EINVAL;
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

    sync = semaphore_create( "select semaphore", 0 );

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
                    semaphore_unlock( requests[ i ].sync, 1 );
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
        semaphore_lock( sync, 1, LOCK_IGNORE_SIGNAL );
    } else {
        uint64_t _timeout;

        _timeout = ( uint64_t )timeout->tv_sec * 1000000 + ( uint64_t )timeout->tv_usec;

        semaphore_timedlock( sync, 1, LOCK_IGNORE_SIGNAL, _timeout );
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

    semaphore_destroy( sync );

    kfree( files );
    kfree( requests );

    return ready_count;
}

int select( int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval_t* timeout ) {
    return do_select( &kernel_io_context, count, readfds, writefds, exceptfds, timeout );
}

int sys_select( int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval_t* timeout ) {
    return do_select( current_process()->io_context, count, readfds, writefds, exceptfds, timeout );
}

static int do_dup( io_context_t* io_context, int old_fd ) {
    int error;
    file_t* file;

    file = io_context_get_file( io_context, old_fd );

    if ( file == NULL ) {
        return -EBADF;
    }

    error = io_context_insert_file( io_context, file, 3 );

    io_context_put_file( io_context, file );

    return error;
}

int dup( int old_fd ) {
    return do_dup( &kernel_io_context, old_fd );
}

int sys_dup( int old_fd ) {
    return do_dup( current_process()->io_context, old_fd );
}

static int do_dup2( io_context_t* io_context, int old_fd, int new_fd ) {
    int error;
    file_t* file;

    file = io_context_get_file( io_context, old_fd );

    if ( file == NULL ) {
        return -EBADF;
    }

    error = io_context_insert_file_at( io_context, file, new_fd, true );

    if ( error < 0 ) {
        goto out;
    }

    error = new_fd;

out:
    io_context_put_file( io_context, file );

    return error;
}

int sys_dup2( int old_fd, int new_fd ) {
    return do_dup2( current_process()->io_context, old_fd, new_fd );
}

static int do_utime( io_context_t* io_context, const char* path, const struct utimbuf* times ) {
    int error;
    inode_t* inode;

    struct stat _stat = {
        .st_atime = times->actime,
        .st_ctime = times->modtime
    };

    error = lookup_inode( io_context, NULL, path, &inode, true, true );

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

int sys_utime( const char* filename, const struct utimbuf* times ) {
    return do_utime( current_process()->io_context, filename, times );
}

__init int init_vfs( void ) {
    int error;

    mount_points = NULL;

    /* Initialize the kernel I/O context */

    error = init_io_context( &kernel_io_context, INIT_FILE_TABLE_SIZE );

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

    error = do_mount( &kernel_io_context, "", "/device", "devfs", MOUNT_NONE );

    if ( error < 0 ) {
        return error;
    }

    return 0;

error1:
    return error;
}
