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

#include <configserver/node.h>
#include <configserver/attribute.h>

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
