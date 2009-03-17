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

#include <errno.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <lib/circular.h>
#include <lib/string.h>

int init_circular_pointer( circular_buffer_t* buffer, circular_pointer_t* pointer, size_t offset ) {
    if ( offset >= buffer->buffer_size ) {
        return -EINVAL;
    }

    pointer->offset = offset;

    return 0;
}

size_t circular_pointer_diff( circular_buffer_t* buffer, circular_pointer_t* ptr1, circular_pointer_t* ptr2 ) {
    if ( ptr1->offset <= ptr2->offset ) {
        return ( ptr2->offset - ptr1->offset );
    }

    return ( buffer->buffer_size - ptr1->offset + ptr2->offset );
}

void circular_pointer_move( circular_buffer_t* buffer, circular_pointer_t* pointer, size_t size ) {
    pointer->offset = ( pointer->offset + size ) % buffer->buffer_size;
}

int init_circular_buffer( circular_buffer_t* buffer, size_t buffer_size ) {
    buffer->buffer = ( uint8_t* )kmalloc( buffer_size );

    if ( buffer->buffer == NULL ) {
        return -ENOMEM;
    }

    buffer->buffer_size = buffer_size;

    return 0;
}

void destroy_circular_buffer( circular_buffer_t* buffer ) {
    kfree( buffer->buffer );
    buffer->buffer = NULL;
}

size_t circular_buffer_size( circular_buffer_t* buffer ) {
    return buffer->buffer_size;
}

void circular_buffer_read( circular_buffer_t* buffer, circular_pointer_t* pointer, void* _data, size_t size ) {
    uint8_t* data;
    size_t to_copy;

    data = ( uint8_t* )_data;
    to_copy = buffer->buffer_size - pointer->offset;
    to_copy = MIN( to_copy, size );

    memcpy( data, buffer->buffer + pointer->offset, to_copy );

    data += to_copy;
    size -= to_copy;

    while ( size > 0 ) {
        to_copy = MIN( size, buffer->buffer_size );

        memcpy( data, buffer->buffer, to_copy );

        data += to_copy;
        size -= to_copy;
    }
}

void circular_buffer_write( circular_buffer_t* buffer, circular_pointer_t* pointer, const void* _data, size_t size ) {
    uint8_t* data;
    size_t to_copy;

    data = ( uint8_t* )_data;
    to_copy = buffer->buffer_size - pointer->offset;
    to_copy = MIN( to_copy, size );

    memcpy( buffer->buffer + pointer->offset, data, to_copy );

    data += to_copy;
    size -= to_copy;

    while ( size > 0 ) {
        to_copy = MIN( size, buffer->buffer_size );

        memcpy( buffer->buffer, data, to_copy );

        data += to_copy;
        size -= to_copy;
    }
}
