/* Inode related definitions and functions
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

#ifndef _VFS_INODE_H_
#define _VFS_INODE_H_

#include <types.h>
#include <semaphore.h>
#include <lib/hashtable.h>

#include <arch/atomic.h>

typedef int64_t ino_t;

struct mount_point;
struct io_context;

typedef struct inode {
    hashitem_t hash;

    ino_t inode_number;
    atomic_t ref_count;
    struct inode* mount;
    struct mount_point* mount_point;
    void* fs_node;

    struct inode* next_free;
} inode_t;

typedef struct inode_cache {
    hashtable_t inode_table;

    int free_inode_count;
    int max_free_inode_count;
    inode_t* free_inodes;

    semaphore_id lock;
} inode_cache_t;

inode_t* get_inode( struct mount_point* mount_point, ino_t inode_number );
int put_inode( inode_t* inode );

int do_lookup_inode(
    struct io_context* io_context,
    inode_t* parent,
    const char* name,
    int name_length,
    bool follow_mount,
    inode_t** result
);

int lookup_parent_inode(
    struct io_context* io_context,
    inode_t* inode,
    const char* path,
    char** name,
    int* length,
    inode_t** _parent
);

int lookup_inode(
    struct io_context* io_context,
    inode_t* parent,
    const char* path,
    inode_t** _inode,
    bool follow_symlink,
    bool follow_mount
);

uint32_t get_inode_cache_size( inode_cache_t* cache );

int init_inode_cache( inode_cache_t* cache, int current_size, int free_inodes, int max_free_inodes );
void destroy_inode_cache( inode_cache_t* cache );

#endif // _VFS_INODE_H_
