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

#ifndef _VFS_VFS_H_
#define _VFS_VFS_H_

#include <types.h>
#include <semaphore.h>
#include <kernel.h>
#include <vfs/inode.h>
#include <vfs/filesystem.h>
#include <vfs/io_context.h>
#include <lib/string.h>

#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR   0x04
#define O_CREAT  0x08

#define NAME_MAX 255

#define FD_ZERO(set) \
    memset( (set)->fds, 0, 1024 / 32 );

#define FD_CLR(fd,set) \
    ASSERT((fd>=0)&&(fd<1024)); \
    (set)->fds[fd/32] &= ~(1<<(fd%32));

#define FD_SET(fd,set) \
    ASSERT((fd>=0)&&(fd<1024)); \
    (set)->fds[fd/32] |= (1<<(fd%32));

#define FD_ISSET(fd,set) \
    ((set)->fds[fd/32] & (1<<(fd%32)))

typedef struct dirent {
    ino_t inode_number;
    char name[ NAME_MAX + 1 ];
} dirent_t;

typedef struct mount_point {
    inode_cache_t inode_cache;
    filesystem_calls_t* fs_calls;
    void* fs_data;
    struct mount_point* next;
} mount_point_t;

typedef enum select_type {
    SELECT_NONE = 0,
    SELECT_READ = 1,
    SELECT_WRITE = 2,
    SELECT_EXCEPT = 4
} select_type_t;

typedef struct select_request {
    semaphore_id sync;
    select_type_t type;
    bool ready;
    int fd;
    struct select_request* next;
} select_request_t;

typedef struct fd_set {
    uint32_t fds[ 1024 / 32 ];
} fd_set;

extern io_context_t kernel_io_context;

mount_point_t* create_mount_point(
    filesystem_calls_t* fs_calls,
    int inode_cache_size,
    int free_inodes,
    int max_free_inodes
);
void delete_mount_point( mount_point_t* mount_point );
int insert_mount_point( mount_point_t* mount_point );

int open( const char* path, int flags );
int close( int fd );
int pread( int fd, void* buffer, size_t count, off_t offset );
int pwrite( int fd, const void* buffer, size_t count, off_t offset );
int ioctl( int fd, int command, void* buffer );
int getdents( int fd, dirent_t* entry );
int mkdir( const char* path, int permissions );
int mount( const char* device, const char* dir, const char* filesystem );
int select( int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, void* timeout );

int sys_open( const char* path, int flags );
int sys_close( int fd );
int sys_read( int fd, void* buffer, size_t count );
int sys_write( int fd, const void* buffer, size_t count );
int sys_dup2( int old_fd, int new_fd );
int sys_isatty( int fd );
int sys_getdents( int fd, dirent_t* entry, unsigned int count );
int sys_fchdir( int fd );

int init_vfs( void );

#endif // _VFS_VFS_H_
