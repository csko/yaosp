#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../../include/yutil/list.h"

#define TST_ASSERT(exp)                              \
    if ( !(exp) ) {                                  \
        printf( "Assertion '" #exp "' failed!\n" );  \
        exit(1);                                     \
    }

/* Empty list test */

static void test_list_1( void ) {
    list_t list;

    TST_ASSERT( init_list( &list ) == 0 );
    TST_ASSERT( list_get_size( &list ) == 0 );
    TST_ASSERT( list_get_head( &list ) == NULL );
    TST_ASSERT( list_get_tail( &list ) == NULL );
    TST_ASSERT( destroy_list( &list ) == 0 );
}

/* Add test */

static void test_list_2( void ) {
    list_t list;
    void* item = ( void* )0x12345678;

    init_list( &list );
    TST_ASSERT( list_add_item( &list, item ) == 0 );
    TST_ASSERT( list_get_size( &list ) == 1 );
    TST_ASSERT( list_get_head( &list ) == item );
    TST_ASSERT( list_get_tail( &list ) == item );
    destroy_list( &list );
}

/* Get head & get tail test */

static void test_list_3( void ) {
    list_t list;
    void* item1 = ( void* )0x12345678;
    void* item2 = ( void* )0x87654321;

    init_list( &list );
    list_add_item( &list, item1 );
    list_add_item( &list, item2 );
    TST_ASSERT( list_get_size( &list ) == 2 );
    TST_ASSERT( list_get_head( &list ) == item1 );
    TST_ASSERT( list_get_tail( &list ) == item2 );
    destroy_list( &list );
}

/* Pop head test */

static void test_list_4( void ) {
    list_t list;
    void* item1 = ( void* )0x12345678;
    void* item2 = ( void* )0x87654321;

    init_list( &list );
    list_add_item( &list, item1 );
    list_add_item( &list, item2 );
    TST_ASSERT( list_get_size( &list ) == 2 );
    TST_ASSERT( list_pop_head( &list ) == item1 );
    TST_ASSERT( list_get_size( &list ) == 1 );
    TST_ASSERT( list_pop_head( &list ) == item2 );
    TST_ASSERT( list_get_size( &list ) == 0 );
    destroy_list( &list );
}

int main( int argc, char** argv ) {
    test_list_1();
    test_list_2();
    test_list_3();
    test_list_4();

    return 0;
}
