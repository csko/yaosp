#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/yutil/blockbuffer.h"

#define TST_ASSERT(exp)                              \
    if ( !(exp) ) {                                  \
        printf( "Assertion '" #exp "' failed!\n" );  \
        exit(1);                                     \
    }

static void test_blockbuffer_1( void ) {
    blockbuffer_t buffer;

    TST_ASSERT( init_block_buffer( &buffer ) == 0 );
    TST_ASSERT( block_buffer_get_size( &buffer ) == 0 );
    TST_ASSERT( destroy_block_buffer( &buffer ) == 0 );
}

int main( int argc, char** argv ) {
    test_blockbuffer_1();

    return 0;
}
