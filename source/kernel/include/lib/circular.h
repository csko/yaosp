/* Circular buffer implementation
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

#ifndef _LIB_CIRCULAR_H_
#define _LIB_CIRCULAR_H_

#include <types.h>

typedef struct circular_buffer {
    uint8_t* buffer;
    size_t buffer_size;
} circular_buffer_t;

typedef struct circular_pointer {
    size_t offset;
} circular_pointer_t;

int init_circular_pointer( circular_buffer_t* buffer, circular_pointer_t* pointer, size_t offset );
size_t circular_pointer_diff( circular_buffer_t* buffer, circular_pointer_t* ptr1, circular_pointer_t* ptr2 );
void circular_pointer_move( circular_buffer_t* buffer, circular_pointer_t* pointer, size_t size );
void* circular_pointer_get( circular_buffer_t* buffer, circular_pointer_t* pointer );

int init_circular_buffer( circular_buffer_t* buffer, size_t buffer_size );
void destroy_circular_buffer( circular_buffer_t* buffer );
size_t circular_buffer_size( circular_buffer_t* buffer );
void circular_buffer_read( circular_buffer_t* buffer, circular_pointer_t* pointer, void* _data, size_t size );
void circular_buffer_write( circular_buffer_t* buffer, circular_pointer_t* pointer, const void* _data, size_t size );

#endif /* _LIB_CIRCULAR_H_ */
