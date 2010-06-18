#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/yutil/string.h"

#define TST_ASSERT(exp)                              \
    if ( !(exp) ) {                                  \
        printf( "Assertion '" #exp "' failed!\n" );  \
        exit(1);                                     \
    }

static void test_string_1( void ) {
    string_t str;

    TST_ASSERT( init_string( &str ) == 0 );
    TST_ASSERT( string_length( &str ) == 0 );
    TST_ASSERT( destroy_string( &str ) == 0 );
}

static void test_string_2( void ) {
    string_t str;

    init_string( &str );
    TST_ASSERT( string_append( &str, "hello", 5 ) == 0 );
    TST_ASSERT( string_length( &str ) == 5 );
    TST_ASSERT( memcmp( str.buffer, "hello", 6 ) == 0 );
    TST_ASSERT( string_append( &str, "world", 5 ) == 0 );
    TST_ASSERT( string_length( &str ) == 10 );
    TST_ASSERT( memcmp( str.buffer, "helloworld", 11 ) == 0 );
    destroy_string( &str );
}

static void test_string_3( void ) {
    string_t str;

    init_string( &str );
    string_append( &str, "held", 4 );
    TST_ASSERT( string_insert( &str, 2, "llowor", 6 ) == 0 );
    TST_ASSERT( string_length( &str ) == 10 );
    TST_ASSERT( memcmp( str.buffer, "helloworld", 11 ) == 0 );
    destroy_string( &str );
}

int main( int argc, char** argv ) {
    test_string_1();
    test_string_2();
    test_string_3();

    return 0;
}
