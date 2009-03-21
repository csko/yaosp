/* I/O context
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

#ifndef _VFS_IO_CONTEXT_H_
#define _VFS_IO_CONTEXT_H_

#include <vfs/inode.h>
#include <lib/hashtable.h>

#include <arch/atomic.h>

typedef enum file_type {
    TYPE_FILE,
    TYPE_DIRECTORY
} file_type_t;

typedef struct file {
    hashitem_t hash;

    int fd;
    int flags;
    off_t position;
    inode_t* inode;
    file_type_t type;
    void* cookie;
    atomic_t ref_count;
} file_t;

typedef struct io_context {
    inode_t* root_directory;
    inode_t* current_directory;
    semaphore_id lock;

    int fd_counter;
    hashtable_t file_table;
} io_context_t;

file_t* create_file( void );
void delete_file( file_t* file );

int io_context_insert_file( io_context_t* io_context, file_t* file );
int io_context_insert_file_with_fd( io_context_t* io_context, file_t* file, int fd );
file_t* io_context_get_file( io_context_t* io_context, int fd );
void io_context_put_file( io_context_t* io_context, file_t* file );

io_context_t* io_context_clone( io_context_t* old_io_context );

int init_io_context( io_context_t* io_context );
void destroy_io_context( io_context_t* io_context );

#endif // _VFS_IO_CONTEXT_H_
