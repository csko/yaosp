/* vsnprintf function
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

#include <stdio.h>

#include "__printf.h"

typedef struct vsnprintf_buffer {
    size_t position;
    size_t size;
    char* buffer;
} vsnprintf_buffer_t;

static int vsnprintf_helper( void* data, char c ) {
    vsnprintf_buffer_t* buffer;

    buffer = ( vsnprintf_buffer_t* )data;

    if ( buffer->position < ( buffer->size - 1 ) ) {
        buffer->buffer[ buffer->position++ ] = c;
    }

    return 0;
}

int vsnprintf( char* str, size_t size, const char* format, va_list args ) {
    int ret;
    vsnprintf_buffer_t data;

    data.position = 0;
    data.size = size;
    data.buffer = str;

    ret = __printf( vsnprintf_helper, &data, format, args );

    str[ data.position ] = 0;

    return ret;
}
