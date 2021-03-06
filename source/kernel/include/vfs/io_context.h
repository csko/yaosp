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

#include <types.h>
#include <vfs/inode.h>
#include <lib/hashtable.h>

#include <arch/atomic.h>

#define INIT_FILE_TABLE_SIZE 1024

typedef enum file_type {
    TYPE_FILE,
    TYPE_DIRECTORY
} file_type_t;

typedef struct file {
    int flags;
    off_t position;
    inode_t* inode;
    file_type_t type;
    void* cookie;
    atomic_t ref_count;
    bool close_on_exec;
} file_t;

typedef struct io_context {
    lock_id mutex;

    inode_t* root_directory;
    inode_t* current_directory;

    file_t** file_table;
    size_t file_table_size;
} io_context_t;

file_t* create_file( void );
void delete_file( file_t* file );

int io_context_insert_file( io_context_t* io_context, file_t* file, int start_fd );
int io_context_insert_file_at( io_context_t* io_context, file_t* file, int fd, bool close_existing );
int io_context_remove_file( io_context_t* io_context, int fd );

file_t* io_context_get_file( io_context_t* io_context, int fd );
void io_context_put_file( io_context_t* io_context, file_t* file );

io_context_t* io_context_clone( io_context_t* old_io_context );

int init_io_context( io_context_t* io_context, size_t file_table_size );
void destroy_io_context( io_context_t* io_context );

#endif // _VFS_IO_CONTEXT_H_
