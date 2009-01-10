/* sprintf function
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

typedef struct sprintf_buffer {
    size_t position;
    char* buffer;
} sprintf_buffer_t;

static int sprintf_helper( void* data, char c ) {
    sprintf_buffer_t* buffer;

    buffer = ( sprintf_buffer_t* )data;

    buffer->buffer[ buffer->position++ ] = c;

    return 0;
}

int sprintf( char* str, const char* format, ... ) {
    int ret;
    va_list args;
    sprintf_buffer_t data;

    data.position = 0;
    data.buffer = str;

    va_start( args, format );
    ret = __printf( sprintf_helper, &data, format, args );
    va_end( args );

    str[ data.position ] = 0;

    return ret;
}
