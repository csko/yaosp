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
            "printf test failed: Expected '%s' but got '%s' for format: '%s'\n", \
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
}

static int printf_test( const char* format, ... ) {
    va_list args;

    va_start( args, format );
    do_printf( printf_test_helper, &data, format, args );
    va_end( args );

    return 0;
}

int main( int argc, char** argv ) {
    INIT_BUFFER

    DO_TEST( "12", "%d", 12 )
    DO_TEST( "-12", "%d", -12 )
    DO_TEST( "12", "%u", 12 )
    DO_TEST( "c", "%x", 12 )
    DO_TEST( "C", "%X", 12 )
    DO_TEST( "hello", "%s", "hello" )

    printf( "printf test OK!\n" );

    return 0;
}
