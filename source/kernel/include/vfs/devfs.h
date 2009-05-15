/* Device file system
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

#ifndef _VFS_DEVFS_H_
#define _VFS_DEVFS_H_

#include <time.h>
#include <vfs/inode.h>
#include <lib/hashtable.h>

struct select_request;

typedef struct device_calls {
    int ( *open )( void* node, uint32_t flags, void** cookie );
    int ( *close )( void* node, void* cookie );
    int ( *ioctl )( void* node, void* cookie, uint32_t command, void* args, bool from_kernel );
    int ( *read )( void* node, void* cookie, void* buffer, off_t position, size_t size );
    int ( *write )( void* node, void* cookie, const void* buffer, off_t position, size_t size );
    int ( *add_select_request )( void* node, void* cookie, struct select_request* request );
    int ( *remove_select_request )( void* node, void* cookie, struct select_request* request );
} device_calls_t;

typedef struct devfs_node {
    hashitem_t hash;

    char* name;
    bool is_directory;
    ino_t inode_number;
    time_t atime;
    time_t mtime;
    time_t ctime;

    device_calls_t* calls;
    void* cookie;

    struct devfs_node* parent;
    struct devfs_node* next_sibling;
    struct devfs_node* first_child;
} devfs_node_t;

typedef struct devfs_dir_cookie {
    int position;
} devfs_dir_cookie_t;

int create_device_node( const char* path, device_calls_t* calls, void* cookie );

int init_devfs( void );

#endif /* _VFS_DEVFS_H_ */
