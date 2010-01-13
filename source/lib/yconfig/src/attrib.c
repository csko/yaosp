/* yaosp configuration library
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

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <yaosp/ipc.h>

#include <yconfig/protocol.h>

extern ipc_port_id ycfg_server_port;
extern ipc_port_id ycfg_reply_port;

extern pthread_mutex_t ycfg_lock;

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
