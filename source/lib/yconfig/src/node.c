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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <yaosp/ipc.h>

#include <yconfig/protocol.h>
#include <yconfig/yconfig.h>

extern ipc_port_id ycfg_server_port;
extern ipc_port_id ycfg_reply_port;

extern pthread_mutex_t ycfg_lock;

int ycfg_list_children( char* path, array_t* children ) {
    int error;
    size_t path_length;
    size_t request_length;
    msg_list_children_t* request;

    size_t reply_size;
    uint8_t* reply_buffer;

    path_length = strlen( path );
    request_length = sizeof( msg_list_children_t ) + path_length + 1;

    request = ( msg_list_children_t* )malloc( request_length );

    if ( request == NULL ) {
        return -ENOMEM;
    }

    request->reply_port = ycfg_reply_port;
    memcpy( request + 1, path, path_length + 1 );

    pthread_mutex_lock( &ycfg_lock );

    error = send_ipc_message( ycfg_server_port, MSG_LIST_NODE_CHILDREN, request, request_length );

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
            if ( reply->count > 0 ) {
                uint32_t i;
                char* name = ( char* )( reply + 1 );
                size_t length;

                for ( i = 0; i < reply->count; i++ ) {
                    length = strlen( name );

                    array_add_item( children, strdup( name ) );

                    name += ( length + 1 );
                }
            }

            error = 0;
        } else {
            error = reply->error;
        }
    }

    free( reply_buffer );

    return error;
}
