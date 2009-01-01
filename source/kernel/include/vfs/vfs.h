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

#include <vfs/inode.h>
#include <vfs/filesystem.h>
#include <vfs/io_context.h>

#define NAME_MAX 255

typedef struct dirent {
    ino_t inode;
    char name[ NAME_MAX + 1 ];
} dirent_t;

typedef struct mount_point {
    inode_cache_t inode_cache;
    filesystem_calls_t* fs_calls;
    void* fs_data;
    struct mount_point* next;
} mount_point_t;

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
int pread( int fd, void* buffer, size_t count, off_t offset );
int getdents( int fd, dirent_t* entry );
int mkdir( const char* path, int permissions );

int init_vfs( void );

#endif // _VFS_VFS_H_
