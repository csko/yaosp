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

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <yaosp/ipc.h>
#include <yaosp/debug.h>

#include <ygui/protocol.h>
#include <ygui/application.h>

ipc_port_id guiserver_port = -1;

ipc_port_id app_reply_port = -1;
ipc_port_id app_client_port = -1;
ipc_port_id app_server_port = -1;

pthread_mutex_t app_lock = PTHREAD_MUTEX_INITIALIZER;

static msg_handler_t* unknown_msg_handler = NULL;

int ymsg_init( ymsg_t* msg, size_t max_size ) {
    msg->buffer = ( uint8_t* )malloc( max_size );

    if ( msg->buffer == NULL ) {
        return -1;
    }

    msg->size = max_size;

    return 0;
}

int application_set_message_handler( msg_handler_t* handler ) {
    unknown_msg_handler = handler;

    return 0;
}

int application_register_window_listener( int get_window_list ) {
    send_ipc_message( app_server_port, MSG_REG_WINDOW_LISTENER, &get_window_list, sizeof( int ) );

    return 0;
}

int application_init( uint32_t flags ) {
    int error;
    struct timespec slp_time;

    msg_create_app_t request;
    msg_create_app_reply_t reply;

    /* Get the guiserver port ... */

    while ( 1 ) {
        error = get_named_ipc_port( "guiserver", &guiserver_port );

        if ( error == 0 ) {
            break;
        }

        /* Wait 200ms ... */

        slp_time.tv_sec = 0;
        slp_time.tv_nsec = 200 * 1000 * 1000;

        nanosleep( &slp_time, NULL );
    }

    /* Register our own application */

    app_client_port = create_ipc_port();

    if ( app_client_port < 0 ) {
        return app_client_port;
    }

    app_reply_port = create_ipc_port();

    if ( app_reply_port < 0 ) {
        return app_reply_port;
    }

    request.reply_port = app_reply_port;
    request.client_port = app_client_port;
    request.flags = flags;

    error = send_ipc_message( guiserver_port, MSG_APPLICATION_CREATE, &request, sizeof( msg_create_app_t ) );

    if ( error < 0 ) {
        return error;
    }

    error = recv_ipc_message( app_reply_port, NULL, &reply, sizeof( msg_create_app_reply_t ), INFINITE_TIMEOUT );

    if ( error < 0 ) {
        return error;
    }

    if ( reply.server_port < 0 ) {
        return reply.server_port;
    }

    app_server_port = reply.server_port;

    return 0;
}

#define MAX_APPLICATION_BUFSIZE 512

int application_receive_msg( ymsg_t* msg, uint64_t timeout ) {
    int error;

    error = recv_ipc_message( app_client_port, &msg->code, msg->buffer, msg->size, timeout );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int application_process_msg( ymsg_t* msg ) {
    switch ( msg->code ) {
        case MSG_APPLICATION_DESTROY :
            send_ipc_message( app_server_port, MSG_APPLICATION_DESTROY, NULL, 0 );
            return 1;

        default : {
            int handled;

            if ( unknown_msg_handler != NULL ) {
                handled = unknown_msg_handler( msg->code, msg->buffer );
            } else {
                handled = -1;
            }

            if ( handled != 0 ) {
                dbprintf( "run_application(): Received unknown message: %x\n", msg->code );
            }

            break;
        }
    }

    return 0;
}

int application_run( void ) {
    int error;
    int running;
    ymsg_t msg;

    if ( ( guiserver_port == -1 ) ||
         ( app_client_port == -1 ) ||
         ( app_reply_port == -1 ) ||
         ( app_server_port == -1 ) ) {
        return -EINVAL;
    }

    if ( ymsg_init( &msg, MAX_APPLICATION_BUFSIZE ) != 0 ) {
        return -ENOMEM;
    }
    running = 1;

    while ( running ) {
        error = application_receive_msg( &msg, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            dbprintf( "application_run(): failed to receive message: %d\n", error );
            continue;
        }

        error = application_process_msg( &msg );

        if ( error > 0 ) {
            running = 0;
        } else if ( error < 0 ) {
            dbprintf( "application_run(): failed to process message.\n" );
        }
    }

    return 0;
}
