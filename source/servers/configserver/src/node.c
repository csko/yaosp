/* Config server
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <stdlib.h>
#include <string.h>
#include <yaosp/debug.h>

#include <configserver/node.h>

int node_add_child( node_t* parent, node_t* child ) {
    return hashtable_add( &parent->children, ( hashitem_t* )child );
}

int node_add_attribute( node_t* parent, attribute_t* attribute ) {
    return hashtable_add( &parent->attributes, ( hashitem_t* )attribute );
}

node_t* node_get_child( node_t* parent, char* name ) {
    return ( node_t* )hashtable_get( &parent->children, ( const void* )name );
}

attribute_t* node_get_attribute( node_t* parent, char* name ) {
    return ( attribute_t* )hashtable_get( &parent->attributes, ( const void* )name );
}

static int node_dump_child_helper( hashitem_t* item, void* data ) {
    int level = *( int* )data;
    node_t* node = ( node_t* )item;

    node_dump( node, level + 1 );

    return 0;
}

static int node_dump_attrib_helper( hashitem_t* item, void* data ) {
    int i;
    int level = *( int* )data;
    attribute_t* attrib = ( attribute_t* )item;

    for ( i = 0; i < ( level + 1 ) * 2; i++ ) {
        dbprintf( " " );
    }

    dbprintf( "%s type=%d\n", attrib->name, attrib->type );

    return 0;
}

int node_dump( node_t* node, int level ) {
    int i;

    for ( i = 0; i < level * 2; i++ ) {
        dbprintf( " " );
    }

    dbprintf( "%s\n", node->name );

    hashtable_iterate( &node->attributes, node_dump_attrib_helper, &level );
    hashtable_iterate( &node->children, node_dump_child_helper, &level );

    return 0;
}

static void* node_key( hashitem_t* item ) {
    node_t* node = ( node_t* )item;
    return ( void* )node->name;
}

node_t* node_create( const char* name ) {
    node_t* node;
    size_t name_len;

    name_len = strlen( name );

    node = ( node_t* )malloc( sizeof( node_t ) + name_len + 1 );

    if ( node == NULL ) {
        goto error1;
    }

    if ( init_hashtable( &node->children, 16, node_key,
                         hash_string, compare_string ) != 0 ) {
        goto error2;
    }

    if ( init_hashtable( &node->attributes, 32, attribute_key,
                         hash_string, compare_string ) != 0 ) {
        goto error3;
    }

    node->name = ( char* )( node + 1 );
    memcpy( node->name, name, name_len );

    return node;

 error3:
    destroy_hashtable( &node->children );

 error2:
    free( node );

 error1:
    return NULL;
}
