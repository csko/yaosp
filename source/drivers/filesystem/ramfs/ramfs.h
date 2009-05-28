/* RAM filesystem implementation
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <types.h>
#include <semaphore.h>
#include <vfs/inode.h>
#include <lib/hashtable.h>

#ifndef _RAMFS_H_
#define _RAMFS_H_

struct ramfs_inode;

typedef struct ramfs_cookie {
    ino_t inode_id_counter;
    hashtable_t inode_table;
    semaphore_id lock;

    struct ramfs_inode* root_inode;
} ramfs_cookie_t;

typedef struct ramfs_dir_cookie {
    int position;
} ramfs_dir_cookie_t;

typedef struct ramfs_file_cookie {
    int open_flags;
} ramfs_file_cookie_t;

typedef struct ramfs_inode {
    hashitem_t hash;

    ino_t inode_number;
    char* name;
    bool is_directory;

    void* data;
    size_t size;
    char* link_path;

    int link_count;
    bool is_loaded;
    time_t atime;
    time_t mtime;
    time_t ctime;

    struct ramfs_inode* parent;
    struct ramfs_inode* first_children;
    struct ramfs_inode* prev_sibling;
    struct ramfs_inode* next_sibling;
} ramfs_inode_t;

#endif /* _RAMFS_H_ */
