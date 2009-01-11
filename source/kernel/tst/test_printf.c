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
#include <stdarg.h>

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

typedef int printf_helper_t( void* data, char c );
int do_printf( printf_helper_t* helper, void* data, const char* format, va_list args );

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
    DO_TEST( "2147483647", "%d", 2147483647 )
    DO_TEST( "4294967295", "%u", 4294967295UL )
    DO_TEST( "2147483647", "%ld", 2147483647 )
    DO_TEST( "4294967295", "%lu", 4294967295UL )
    DO_TEST( "9223372036854775807", "%lld", 9223372036854775807LL )
    DO_TEST( "18446744073709551615", "%llu", 18446744073709551615ULL )
    DO_TEST( "-12", "%d", -12 )
    DO_TEST( "12", "%u", 12 )
    DO_TEST( "c", "%x", 12 )
    DO_TEST( "C", "%X", 12 )

    DO_TEST( "12", "%0d", 12 )
    DO_TEST( "12", "%1d", 12 )
    DO_TEST( "12", "%2d", 12 )
    DO_TEST( " 12", "%3d", 12 )
    DO_TEST( "  12", "%4d", 12 )
    DO_TEST( "              12", "%16d", 12 )
    DO_TEST( "12", "%0d", 12 )
    DO_TEST( "12", "%00d", 12 )
    DO_TEST( "12", "%000d", 12 )
    DO_TEST( "12", "%01d", 12 )
    DO_TEST( "12", "%02d", 12 )
    DO_TEST( "012", "%03d", 12 )
    DO_TEST( "0012", "%004d", 12 )
    DO_TEST( "12", "%-d", 12 )
    DO_TEST( "12", "%-0d", 12 )
//    DO_TEST( "12", "%0-d", 12 )
    DO_TEST( "12", "%-1d", 12 )
    DO_TEST( "12", "%-2d", 12 )
    DO_TEST( "12 ", "%-3d", 12 )
    DO_TEST( "12  ", "%-4d", 12 )

    DO_TEST( "ab", "%0s", "ab" )
    DO_TEST( "ab", "%1s", "ab" )
    DO_TEST( "ab", "%2s", "ab" )
    DO_TEST( " ab", "%3s", "ab" )
    DO_TEST( "  ab", "%4s", "ab" )
    DO_TEST( "              ab", "%16s", "ab" )
    DO_TEST( "%s", "%%s", "hello" )

    DO_TEST( "ff", "%x", 255 )
    DO_TEST( "FF", "%X", 255 )
    DO_TEST( "ffff", "%x", 65535 )
    DO_TEST( "FFFF", "%X", 65535 )
    DO_TEST( "    ffff", "%8x", 65535 )
    DO_TEST( "    FFFF", "%8X", 65535 )
    DO_TEST( "0000ffff", "%08x", 65535 )
    DO_TEST( "0000FFFF", "%08X", 65535 )
    DO_TEST( "ffff    ", "%-8x", 65535 )
    DO_TEST( "FFFF    ", "%-8X", 65535 )

    DO_TEST( "-120", "%+d", -120 )
    DO_TEST( "+120", "%+d", 120 )
    DO_TEST( "+ff", "%+x", 255 )
    DO_TEST( "+FF", "%+X", 255 )
//    DO_TEST( "+ff", "%+x", -255 )
//    DO_TEST( "+FF", "%+X", -255 )

    DO_TEST( "hello", "%s", "hello" )
    DO_TEST( "%hello", "%%%s", "hello" )
    DO_TEST( "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong", "%s", "longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong" )
    DO_TEST( "  hello world 12 c", "%c %s %s %d %x", ' ', "hello", "world", 12, 12 )

    printf( "OK!\n" );

    return 0;
}
