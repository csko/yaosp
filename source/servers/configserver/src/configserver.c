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
#include <errno.h>
#include <string.h>
#include <yaosp/debug.h>
#include <yaosp/ipc.h>

#include <yconfig/protocol.h>
#include <yutil/array.h>

#include <configserver/storage.h>

static char* argv0 = NULL;
static node_t* config_root = NULL;

static uint8_t recv_buffer[ 8192 ];
static ipc_port_id server_port = -1;

static node_t* find_node_by_path( char* path ) {
    char* start = path;
    char* chunk_end;
    node_t* current_node = config_root;

    do {
        chunk_end = strchr( start, '/' );

        if ( chunk_end != NULL ) {
            *chunk_end = 0;
        }

        current_node = node_get_child( current_node, start );

        if ( current_node == NULL ) {
            return NULL;
        }

        start = chunk_end + 1;
    } while ( chunk_end != NULL );

    return current_node;
}

static int handle_get_attribute_value( msg_get_attr_t* request ) {
    char* path;
    char* attr_name;
    size_t reply_size;
    msg_get_reply_t* reply;
    msg_get_reply_t error_reply;

    path = ( char* )( request + 1 );
    attr_name = path + strlen( path ) + 1;

    node_t* current_node = find_node_by_path( path );

    if ( current_node == NULL ) {
        error_reply.error = -EINVAL;
        goto send_error_reply;
    }

    attribute_t* attrib = node_get_attribute( current_node, attr_name );

    if ( attrib == NULL ) {
        error_reply.error = -EINVAL;
        goto send_error_reply;;
    }

    switch ( attrib->type ) {
        case NUMERIC :
            reply_size = sizeof( msg_get_reply_t ) + sizeof( uint64_t );
            break;

        case ASCII :
            reply_size = sizeof( msg_get_reply_t ) + attrib->value.ascii.size + 1;
            break;

        case BOOL :
            reply_size = sizeof( msg_get_reply_t ) + sizeof( uint8_t );
            break;

        case BINARY :
            reply_size = sizeof( msg_get_reply_t ) + attrib->value.binary.size;
            break;
    }

    reply = ( msg_get_reply_t* )malloc( reply_size );

    if ( reply == NULL ) {
        error_reply.error = -ENOMEM;
        goto send_error_reply;
    }

    reply->error = 0;
    reply->type = attrib->type;
    error_reply.error = binary_storage.get_attribute_value( attrib, ( void* )( reply + 1 ) );

    if ( error_reply.error != 0 ) {
        goto send_error_reply;
    }

    send_ipc_message( request->reply_port, 0, reply, reply_size );

    free( reply );

    return 0;

 send_error_reply:
    send_ipc_message( request->reply_port, 0, &error_reply, sizeof( msg_get_reply_t ) );

    return 0;
}

static int list_node_children_helper( hashitem_t* item, void* data ) {
    node_t* child;
    array_t* children;

    child = ( node_t* )item;
    children = ( array_t* )data;

    array_add_item( children, strdup( child->name ) );

    return 0;
}

static int handle_node_add_child( msg_add_child_t* request ) {
    char* path;
    char* child_name;
    node_t* child;
    node_t* parent;
    msg_add_child_reply_t reply;

    path = (char*)( request + 1 );
    child_name = path + strlen(path) + 1;

    parent = find_node_by_path(path);

    if ( parent == NULL ) {
        reply.error = -EINVAL;
        goto send_reply;
    }

    if ( node_get_child( parent, child_name ) != NULL ) {
        reply.error = -EEXIST;
        goto send_reply;
    }

    child = node_create(child_name);

    if ( child == NULL ) {
        reply.error = -ENOMEM;
        goto send_reply;
    }

    reply.error = node_add_child( parent, child );

    if ( reply.error != 0 ) {
        /* todo: free the child node. */
    }

 send_reply:
    send_ipc_message( request->reply_port, 0, &reply, sizeof(msg_add_child_reply_t) );

    return 0;
}

static int handle_node_list_children( msg_list_children_t* request ) {
    int i;
    int size;
    char* path;
    node_t* node;
    array_t children;
    size_t reply_size;
    msg_list_children_reply_t* reply;
    msg_list_children_reply_t err_reply;

    path = ( char* )( request + 1 );
    node = find_node_by_path( path );

    if ( node == NULL ) {
        err_reply.error = -EINVAL;
        goto send_error_reply;
    }

    if ( init_array( &children ) != 0 ) {
        err_reply.error = -ENOMEM;
        goto send_error_reply;
    }

    hashtable_iterate( &node->children, list_node_children_helper, ( void* )&children );

    size = array_get_size( &children );
    reply_size = sizeof( msg_list_children_reply_t );

    for ( i = 0; i < size; i++ ) {
        char* name = ( char* )array_get_item( &children, i );

        reply_size += ( strlen( name ) + 1 );
    }

    reply = ( msg_list_children_reply_t* )malloc( reply_size );

    if ( reply == NULL ) {
        /* todo: free the array */
        err_reply.error = -ENOMEM;
        goto send_error_reply;
    }

    char* dest_name = ( char* )( reply + 1 );

    for ( i = 0; i < size; i++ ) {
        char* name = ( char* )array_get_item( &children, i );
        size_t name_length = strlen( name );

        memcpy( dest_name, name, name_length + 1 );
        dest_name += ( name_length + 1 );
        free( name );
    }

    destroy_array( &children );

    reply->error = 0;
    reply->count = size;

    send_ipc_message( request->reply_port, 0, reply, reply_size );

    return 0;

 send_error_reply:
    send_ipc_message( request->reply_port, 0, &err_reply, sizeof( msg_list_children_reply_t ) );

    return 0;
}

static int configserver_mainloop( void ) {
    server_port = create_ipc_port();

    if ( server_port < 0 ) {
        return -1;
    }

    if ( register_named_ipc_port( "configserver", server_port ) != 0 ) {
        return -1;
    }

    while ( 1 ) {
        int error;
        uint32_t code;

        error = recv_ipc_message( server_port, &code, recv_buffer, sizeof( recv_buffer ), 1 * 1000 * 1000 );

        if ( error == -ETIME ) {
            continue;
        }

        if ( error < 0 ) {
            dbprintf( "configserver_mainloop(): failed to receive message!\n" );
            break;
        }

        switch ( code ) {
            case MSG_GET_ATTRIBUTE_VALUE :
                handle_get_attribute_value( ( msg_get_attr_t* )recv_buffer );
                break;

            case MSG_NODE_ADD_CHILD :
                handle_node_add_child( ( msg_add_child_t* )recv_buffer );
                break;

            case MSG_NODE_LIST_CHILDREN :
                handle_node_list_children( ( msg_list_children_t* )recv_buffer );
                break;

            default :
                dbprintf( "configserver_mainloop(): invalid message: %x\n", code );
                break;
        }
    }

    return 0;
}

int main( int argc, char** argv ) {
    argv0 = argv[ 0 ];

    if ( argc != 2 ) {
        dbprintf( "%s: invalid command line.\n", argv0 );
        return EXIT_FAILURE;
    }

    if ( binary_storage.load( argv[ 1 ], &config_root ) != 0 ) {
        dbprintf( "%s: failed to load configuration: %s.\n", argv0, argv[ 1 ] );
        return EXIT_FAILURE;
    }

    configserver_mainloop();

    return EXIT_SUCCESS;
}
