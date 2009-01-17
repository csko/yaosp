/* Process filesystem implementation
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#ifndef _PROCFS_H_
#define _PROCFS_H_

#include <types.h>
#include <vfs/inode.h>

typedef struct procfs_node {
    hashitem_t hash;
    ino_t inode_number;
    bool is_directory;
    char* name;
    char* data;
    size_t data_size;
    struct procfs_node* name_node;
    struct procfs_node* next_sibling;
    struct procfs_node* first_child;
} procfs_node_t;

typedef struct procfs_dir_cookie {
    int position;
} procfs_dir_cookie_t;

typedef struct procfs_thr_iter_data {
    process_id proc_id;
    procfs_node_t* proc_node;
} procfs_thr_iter_data_t;

#endif // _PROCFS_H_
