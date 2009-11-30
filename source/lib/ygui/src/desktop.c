/* yaosp GUI library
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <yaosp/ipc.h>

#include <ygui/desktop.h>
#include <ygui/protocol.h>

extern ipc_port_id app_reply_port;
extern ipc_port_id app_server_port;

extern pthread_mutex_t app_lock;

int desktop_get_size( point_t* size ) {
    int error;
    msg_desk_get_size_t request;
    msg_desk_get_size_reply_t reply;

    if ( size == NULL ) {
        return -EINVAL;
    }

    request.reply_port = app_reply_port;
    request.desktop = -1; /* todo */

    pthread_mutex_lock( &app_lock );

    error = send_ipc_message( app_server_port, MSG_DESK_GET_SIZE, &request, sizeof( msg_desk_get_size_t ) );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );

        goto error1;
    }

    error = recv_ipc_message( app_reply_port, NULL, &reply, sizeof( msg_desk_get_size_reply_t ), INFINITE_TIMEOUT );

    pthread_mutex_unlock( &app_lock );

    if ( error < 0 ) {
        goto error1;
    }

    point_copy( size, &reply.size );

    return 0;

 error1:
    return error;
}
