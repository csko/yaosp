/* Configuration handling functions
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

#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <yaosp/ipc.h>

#include <yconfig/protocol.h>

ipc_port_id ycfg_server_port = -1;
ipc_port_id ycfg_reply_port = -1;

pthread_mutex_t ycfg_lock = PTHREAD_MUTEX_INITIALIZER;

static int get_attribute_value( char* path, char* attrib, uint8_t** _reply ) {
    int error;
    size_t path_length;
    size_t attrib_length;
    size_t request_length;
    msg_get_attr_t* request;

    uint8_t* reply;
    size_t reply_size;

    path_length = strlen( path );
    attrib_length = strlen( attrib );
    request_length = sizeof( msg_get_attr_t ) + path_length + attrib_length + 2;

    request = ( msg_get_attr_t* )malloc( request_length );

    if ( request == NULL ) {
        return -ENOMEM;
    }

    request->reply_port = ycfg_reply_port;
    memcpy( request + 1, path, path_length + 1 );
    memcpy( ( char* )( request + 1 ) + path_length + 1, attrib, attrib_length + 1 );

    pthread_mutex_lock( &ycfg_lock );

    error = send_ipc_message( ycfg_server_port, MSG_GET_ATTRIBUTE_VALUE, request, request_length );

    free( request );

    if ( error < 0 ) {
        goto out;
    }

    error = peek_ipc_message( ycfg_reply_port, NULL, &reply_size, INFINITE_TIMEOUT );

    if ( error < 0 ) {
        goto out;
    }

    reply = ( uint8_t* )malloc( reply_size );

    if ( reply == NULL ) {
        error = -ENOMEM;
        goto out;
    }

    error = recv_ipc_message( ycfg_reply_port, NULL, reply, reply_size, 0 );
    *_reply = reply;

 out:
    pthread_mutex_unlock( &ycfg_lock );

    return error;
}

int ycfg_get_ascii_value( char* path, char* attrib, char** value ) {
    int error;
    msg_get_reply_t* reply;

    if ( ycfg_server_port == -1 ) {
        return -EINVAL;
    }

    error = get_attribute_value( path, attrib, ( uint8_t** )&reply );

    if ( error >= 0 ) {
        if ( reply->error == 0 ) {
            if ( reply->type == ASCII ) {
                *value = strdup( ( char* )( reply + 1 ) );
                error = 0;
            } else {
                error = -EINVAL;
            }
        } else {
            error = reply->error;
        }

        free( reply );
    }

    return error;
}

int ycfg_get_numeric_value( char* path, char* attrib, uint64_t* value ) {
    int error;
    msg_get_reply_t* reply;

    if ( ycfg_server_port == -1 ) {
        return -EINVAL;
    }

    error = get_attribute_value( path, attrib, ( uint8_t** )&reply );

    if ( error >= 0 ) {
        if ( reply->error == 0 ) {
            if ( reply->type == NUMERIC ) {
                *value = *( uint64_t* )( reply + 1 );
                error = 0;
            } else {
                error = -EINVAL;
            }
        } else {
            error = reply->error;
        }

        free( reply );
    }

    return error;
}

int ycfg_get_binary_value( char* path, char* attrib, void** _data, size_t* _size ) {
    int error;
    msg_get_reply_t* reply;

    if ( ycfg_server_port == -1 ) {
        return -EINVAL;
    }

    error = get_attribute_value( path, attrib, ( uint8_t** )&reply );

    if ( error >= 0 ) {
        if ( reply->error == 0 ) {
            if ( reply->type == BINARY ) {
                size_t size = error - sizeof( msg_get_reply_t );
                void* data = malloc( size );

                if ( data == NULL ) {
                    error = -ENOMEM;
                } else {
                    memcpy( data, reply + 1, size );

                    *_data = data;
                    *_size = size;

                    error = 0;
                }
            } else {
                error = -EINVAL;
            }
        } else {
            error = reply->error;
        }

        free( reply );
    }

    return error;
}

int ycfg_add_child( char* path, char* child ) {
    int error;
    char* tmp;
    size_t path_length;
    size_t child_length;
    size_t request_length;
    msg_add_child_t* request;
    msg_add_child_reply_t reply;

    path_length = strlen(path);
    child_length = strlen(child);
    request_length = sizeof(msg_add_child_t) + path_length + 1 + child_length + 1;

    request = (msg_add_child_t*)malloc( request_length );

    if ( request == NULL ) {
        return -ENOMEM;
    }

    request->reply_port = ycfg_reply_port;
    strcpy( (char*)(request + 1), path );
    tmp = (char*)(request + 1) + path_length + 1;
    strcpy( tmp, child );

    pthread_mutex_lock( &ycfg_lock );

    error = send_ipc_message( ycfg_server_port, MSG_NODE_ADD_CHILD, request, request_length );

    free( request );

    if ( error < 0 ) {
        goto out;
    }

    error = recv_ipc_message( ycfg_reply_port, NULL, &reply, sizeof(msg_add_child_reply_t), INFINITE_TIMEOUT );

 out:
    pthread_mutex_unlock( &ycfg_lock );

    if ( error >= 0 ) {
        return reply.error;
    }

    return error;
}

int ycfg_list_children( char* path, char*** _children ) {
    int error;
    size_t path_length;
    size_t request_length;
    msg_list_children_t* request;

    size_t reply_size;
    uint8_t* reply_buffer = NULL;

    path_length = strlen( path );
    request_length = sizeof( msg_list_children_t ) + path_length + 1;

    request = ( msg_list_children_t* )malloc( request_length );

    if ( request == NULL ) {
        return -ENOMEM;
    }

    request->reply_port = ycfg_reply_port;
    strcpy( (char*)(request + 1), path );

    pthread_mutex_lock( &ycfg_lock );

    error = send_ipc_message( ycfg_server_port, MSG_NODE_LIST_CHILDREN, request, request_length );

    free( request );

    if ( error < 0 ) {
        goto out;
    }

    error = peek_ipc_message( ycfg_reply_port, NULL, &reply_size, INFINITE_TIMEOUT );

    if ( error < 0 ) {
        goto out;
    }

    reply_buffer = ( uint8_t* )malloc( reply_size );

    if ( reply_buffer == NULL ) {
        error = -ENOMEM;
        goto out;
    }

    error = recv_ipc_message( ycfg_reply_port, NULL, reply_buffer, reply_size, 0 );

 out:
    pthread_mutex_unlock( &ycfg_lock );

    if ( error >= 0 ) {
        msg_list_children_reply_t* reply;

        reply = ( msg_list_children_reply_t* )reply_buffer;

        if ( reply->error == 0 ) {
            char** children;
            uint32_t i;
            char* name = ( char* )( reply + 1 );
            size_t length;

            children = ( char** )malloc( sizeof(char*) * ( reply->count + 1 ) );

            if ( children == NULL ) {
                error = -ENOMEM;
                goto out2;
            }

            for ( i = 0; i < reply->count; i++ ) {
                length = strlen( name );
                children[i] = strdup(name);

                name += ( length + 1 );
            }

            children[ reply->count ] = NULL;
            *_children = children;

            error = 0;
        } else {
            error = reply->error;
        }
    }

 out2:
    free( reply_buffer );

    return error;
}

int ycfg_init( void ) {
    if ( ycfg_server_port != -1 ) {
        return 0;
    }

    /* Get the configserver port ... */

    while ( 1 ) {
        int error;
        struct timespec slp_time;

        error = get_named_ipc_port( "configserver", &ycfg_server_port );

        if ( error == 0 ) {
            break;
        }

        /* Wait 200ms ... */

        slp_time.tv_sec = 0;
        slp_time.tv_nsec = 200 * 1000 * 1000;

        nanosleep( &slp_time, NULL );
    }

    /* Create a reply port */

    ycfg_reply_port = create_ipc_port();

    if ( ycfg_reply_port < 0 ) {
        return -1;
    }

    return 0;
}
