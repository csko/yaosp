/* Root file system
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

#ifndef _VFS_ROOTFS_H_
#define _VFS_ROOTFS_H_

#include <vfs/inode.h>
#include <time.h>

typedef struct rootfs_node {
    hashitem_t hash;

    char* name;
    bool is_directory;
    ino_t inode_number;
    time_t atime;
    time_t mtime;
    time_t ctime;
    char* link_path;

    bool is_loaded;
    int link_count;

    struct rootfs_node* parent;
    struct rootfs_node* prev_sibling;
    struct rootfs_node* next_sibling;
    struct rootfs_node* first_child;
} rootfs_node_t;

typedef struct rootfs_dir_cookie {
    int position;
} rootfs_dir_cookie_t;

int init_root_filesystem( void );

#endif /* _VFS_ROOTFS_H_ */
