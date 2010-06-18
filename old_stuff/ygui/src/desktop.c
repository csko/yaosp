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
#include <stdlib.h>
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

array_t* desktop_get_screen_modes( void ) {
    int error;
    uint32_t i;
    size_t reply_size;
    array_t* mode_list;
    screen_mode_info_t* mode_info;
    msg_scr_mode_list_t* reply;
    msg_scr_get_modes_t request;

    request.reply_port = app_reply_port;

    pthread_mutex_lock( &app_lock );

    error = send_ipc_message( app_server_port, MSG_SCREEN_GET_MODES, &request, sizeof( msg_scr_get_modes_t ) );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );
        goto error1;
    }

    error = peek_ipc_message( app_reply_port, NULL, &reply_size, INFINITE_TIMEOUT );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );
        goto error1;
    }

    reply = ( msg_scr_mode_list_t* )malloc( reply_size );

    if ( reply == NULL ) {
        pthread_mutex_unlock( &app_lock );
        error = -ENOMEM;
        goto error1;
    }

    error = recv_ipc_message( app_reply_port, NULL, reply, reply_size, 0 );

    pthread_mutex_unlock( &app_lock );

    if ( error < 0 ) {
        free( reply );
        goto error1;
    }

    mode_list = ( array_t* )malloc( sizeof( array_t ) );

    if ( mode_list == NULL ) {
        free( reply );
        goto error1;
    }

    if ( init_array( mode_list ) != 0 ) {
        free( mode_list );
        free( reply );
        goto error1;
    }

    mode_info = ( screen_mode_info_t* )( reply + 1 );

    for ( i = 0; i < reply->mode_count; i++, mode_info++ ) {
        screen_mode_info_t* tmp;

        tmp = ( screen_mode_info_t* )malloc( sizeof( screen_mode_info_t ) );
        memcpy( tmp, mode_info, sizeof( screen_mode_info_t ) );

        array_add_item( mode_list, tmp );
    }

    free( reply );

    return mode_list;

 error1:
    return NULL;
}

int desktop_put_screen_modes( array_t* mode_list ) {
    int i;
    int size;

    size = array_get_size( mode_list );

    for ( i = 0; i < size; i++ ) {
        free( array_get_item( mode_list, i ) );
    }

    destroy_array( mode_list );
    free( mode_list );

    return 0;
}

int desktop_set_screen_mode( screen_mode_info_t* mode_info ) {
    int error;
    msg_scr_set_mode_t request;

    if ( mode_info == NULL ) {
        return -EINVAL;
    }

    request.reply_port = app_reply_port;
    memcpy( &request.mode_info, mode_info, sizeof( screen_mode_info_t ) );

    pthread_mutex_lock( &app_lock );

    error = send_ipc_message( app_server_port, MSG_SCREEN_SET_MODE, &request, sizeof( msg_scr_set_mode_t ) );

    if ( error < 0 ) {
        pthread_mutex_unlock( &app_lock );
        goto error1;
    }

    error = recv_ipc_message( app_reply_port, NULL, NULL, 0, INFINITE_TIMEOUT );

    pthread_mutex_unlock( &app_lock );

    if ( error < 0 ) {
        goto error1;
    }

    return 0;

 error1:
    return error;
}
