/* snprintf implementation
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

#include <lib/printf.h>

typedef struct snprintf_buffer {
    char* buffer;
    size_t size;
    size_t position;
} snprintf_buffer_t;

static int snprintf_helper( void* data, char c ) {
    snprintf_buffer_t* buffer;

    buffer = ( snprintf_buffer_t* )data;

    if ( buffer->position < ( buffer->size - 1 ) ) {
        buffer->buffer[ buffer->position++ ] = c;
    }

    return 0;
}

int snprintf( char* str, size_t size, const char* format, ... ) {
    va_list args;
    snprintf_buffer_t buffer;

    buffer.buffer = str;
    buffer.size = size;
    buffer.position = 0;

    va_start( args, format );
    do_printf( snprintf_helper, ( void* )buffer, format, args );
    va_end( args );

    str[ buffer.position ] = 0;

    return 0;
}
