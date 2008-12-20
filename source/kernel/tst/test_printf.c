/* Test module for printf()
 *
 * Copyright (c) 2008 Zoltan Kovacs, Kornel Csernai
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
#include <stdlib.h>
#include <string.h>
#include <lib/printf.h>

#define INIT_BUFFER \
    data.buffer = buffer; \
    data.position = 0;

#define DO_TEST( exp, format, arg... ) \
    data.position = 0; \
    printf_test( format, ##arg ); \
    data.buffer[ data.position ] = 0; \
    if ( strcmp( buffer, exp ) != 0 ) { \
        printf( \
            "test failed: Expected '%s' but got '%s' for format: '%s'\n", \
            exp, \
            buffer, \
            format \
        ); \
        exit( 1 ); \
    }

typedef struct printf_buffer {
    char* buffer;
    size_t position;
} printf_buffer_t;

static char buffer[ 512 ];
static printf_buffer_t data;

static int printf_test_helper( void* data, char c ) {
    printf_buffer_t* buffer;

    buffer = ( printf_buffer_t* )data;

    buffer->buffer[ buffer->position++ ] = c;
    return 0;
}

static int printf_test( const char* format, ... ) {
    va_list args;

    va_start( args, format );
    do_printf( printf_test_helper, &data, format, args );
    va_end( args );

    return 0;
}

int main( int argc, char** argv ) {
    printf("Doing a few printf tests... ");
    INIT_BUFFER

    DO_TEST( "A", "%c", 'A' )
    DO_TEST( "12", "%d", 12 )
    DO_TEST( "-12", "%d", -12 )
    DO_TEST( "12", "%u", 12 )
    DO_TEST( "c", "%x", 12 )
    DO_TEST( "C", "%X", 12 )
    DO_TEST( "hello", "%s", "hello" )
    DO_TEST( "  hello world 12 c", "%c %s %s %d %x", ' ', "hello", "world", 12, 12 )

    printf( "OK!\n" );

    return 0;
}
