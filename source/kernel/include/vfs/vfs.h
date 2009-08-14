/* Virtual file system
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs, Kornel Csernai
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
#include <lock/condition.h>
#include <kernel.h>
#include <time.h> /* timeval_t */
#include <vfs/inode.h>
#include <vfs/filesystem.h>
#include <vfs/io_context.h>
#include <lib/string.h>

#define O_RDONLY   0x01
#define O_WRONLY   0x02
#define O_RDWR     0x03
#define O_CREAT    0x04
#define O_TRUNC    0x08
#define O_APPEND   0x10
#define O_EXCL     0x20
#define O_NONBLOCK 0x40

#define F_DUPFD 0
#define F_GETFD 1
#define F_SETFD 2
#define F_GETFL 3
#define F_SETFL 4

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

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* stat definitions */

#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000

/* write_stat bitmasks */

#define WSTAT_DEV     ( 1 << 0 )
#define WSTAT_INO     ( 1 << 1 )
#define WSTAT_MODE    ( 1 << 2 )
#define WSTAT_NLINK   ( 1 << 3 )
#define WSTAT_UID     ( 1 << 4 )
#define WSTAT_GID     ( 1 << 5 )
#define WSTAT_RDEV    ( 1 << 6 )
#define WSTAT_SIZE    ( 1 << 7 )
#define WSTAT_BLKSIZE ( 1 << 8 )
#define WSTAT_BLOCKS  ( 1 << 9 )
#define WSTAT_ATIME   ( 1 << 10 )
#define WSTAT_MTIME   ( 1 << 11 )
#define WSTAT_CTIME   ( 1 << 12 )
#define WSTAT_ALL     \
    ( WSTAT_DEV | WSTAT_INO | WSTAT_MODE | WSTAT_NLINK  \
      WSTAT_UID | WSTAT_GID | WSTAT_RDEV | WSTAT_SIZE \
      WSTAT_BLKSIZE | WSTAT_BLOCKS | WSTAT_ATIME | WSTAT_MTIME \
      WSTAT_CTIME )

typedef struct dirent {
    ino_t inode_number;
    char name[ NAME_MAX + 1 ];
} dirent_t;

typedef struct mount_point {
    inode_cache_t inode_cache;
    filesystem_calls_t* fs_calls;
    ino_t root_inode_number;
    inode_t* mount_inode;
    void* fs_data;
    struct mount_point* next;
    uint32_t flags;
} mount_point_t;

typedef enum select_type {
    SELECT_NONE = 0,
    SELECT_READ = 1,
    SELECT_WRITE = 2,
    SELECT_EXCEPT = 4
} select_type_t;

typedef struct select_request {
    lock_id sync;
    select_type_t type;
    bool ready;
    int fd;
    struct select_request* next;
} select_request_t;

typedef struct fd_set {
    uint32_t fds[ 1024 / 32 ];
} fd_set;

struct stat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    dev_t st_rdev;
    off_t st_size;
    blksize_t st_blksize;
    blkcnt_t st_blocks;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
};

struct utimbuf {
    time_t actime; /* access time */
    time_t modtime; /* modification time */
};

extern io_context_t kernel_io_context;

mount_point_t* create_mount_point(
    filesystem_calls_t* fs_calls,
    int inode_cache_size,
    int free_inodes,
    int max_free_inodes,
    uint32_t flags
);
void delete_mount_point( mount_point_t* mount_point );
int insert_mount_point( mount_point_t* mount_point );
mount_point_t* get_mount_point_by_cookie( void* cookie );
int mount_point_can_unmount( mount_point_t* mount_point );

int follow_symbolic_link( io_context_t* io_context, inode_t** _parent, inode_t** _inode );

int open( const char* path, int flags );
int close( int fd );
int do_pread_helper( file_t* file, void* buffer, size_t count, off_t offset );
int pread( int fd, void* buffer, size_t count, off_t offset );
int pwrite( int fd, const void* buffer, size_t count, off_t offset );
int ioctl( int fd, int command, void* buffer );
int getdents( int fd, dirent_t* entry, unsigned int count );
int fstat( int fd, struct stat* stat );
off_t lseek( int fd, off_t offset, int whence );
int dup( int old_fd );
int mkdir( const char* path, int permissions );
int symlink( const char* src, const char* dest );
int mount( const char* device, const char* dir, const char* filesystem, uint32_t mountflags );
int select( int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval_t* timeout );

int sys_open( const char* path, int flags );
int sys_close( int fd );
int sys_read( int fd, void* buffer, size_t count );
int sys_write( int fd, const void* buffer, size_t count );
int sys_pread( int fd, void* buffer, size_t count, off_t* offset );
int sys_pwrite( int fd, const void* buffer, size_t count, off_t* offset );
int sys_dup( int old_fd );
int sys_dup2( int old_fd, int new_fd );
int sys_isatty( int fd );
int sys_ioctl( int fd, int command, void* buffer );
int sys_getdents( int fd, dirent_t* entry, unsigned int count );
int sys_rewinddir( int fd );
int sys_chdir( const char* path );
int sys_fchdir( int fd );
int sys_stat( const char* path, struct stat* stat );
int sys_lstat( const char* path, struct stat* stat );
int sys_fstat( int fd, struct stat* stat );
int sys_lseek( int fd, off_t* offset, int whence, off_t* result );
int sys_fcntl( int fd, int cmd, int arg );
int sys_access( const char* path, int mode );
int sys_mkdir( const char* path, int perms );
int sys_rmdir( const char* path );
int sys_unlink( const char* path );
int sys_symlink( const char* src, const char* dest );
int sys_readlink( const char* path, char* buffer, size_t length );
int sys_mount( const char* device, const char* dir, const char* filesystem, uint32_t mountflags );
int sys_unmount( const char* dir );
int sys_select( int count, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval_t* timeout );
int sys_utime( const char* filename, const struct utimbuf* times );

int init_vfs( void );

#endif /* _VFS_VFS_H_ */
