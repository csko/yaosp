/* Blockbuffer implementation
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

#ifndef _YUTIL_BLOCKBUFFER_H_
#define _YUTIL_BLOCKBUFFER_H_

#include <yutil/list.h>

typedef struct block {
    int size;
} block_t;

typedef struct block_buffer {
    int block_size;

    int size;
    list_t used_blocks;

    int max_free;
    list_t free_blocks;
} block_buffer_t;

int block_buffer_read_and_delete( block_buffer_t* buffer, void* data, size_t size );
int block_buffer_write( block_buffer_t* buffer, void* data, size_t size );
int block_buffer_get_size( block_buffer_t* buffer );

int block_buffer_set_max_free( block_buffer_t* buffer, int max_free );

int init_block_buffer( block_buffer_t* buffer, int block_size );
int destroy_block_buffer( block_buffer_t* buffer );

#endif /* _YUTIL_BLOCKBUFFER_H_ */
