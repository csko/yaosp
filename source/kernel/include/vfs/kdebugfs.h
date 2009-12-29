/* Kernel debug filesystem
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

#ifndef _VFS_KDEBUGFS_H_
#define _VFS_KDEBUGFS_H_

#include <vfs/inode.h>
#include <lib/hashtable.h>

typedef struct kdbgfs_node {
    hashitem_t hash;

    char* name;
    ino_t inode_number;

    char* buffer;
    size_t buffer_size;
    size_t max_buffer_size;
} kdbgfs_node_t;

typedef struct kdbgfs_dir_cookie {
    int position;
} kdbgfs_dir_cookie_t;

kdbgfs_node_t* kdebugfs_create_node( const char* name, size_t max_buffer_size );
int kdebugfs_write_node( kdbgfs_node_t* node, void* data, size_t size );

int init_kdebugfs( void );

#endif /* _VFS_KDEBUGFS_H_ */
