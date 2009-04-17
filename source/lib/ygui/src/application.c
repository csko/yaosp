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
#include <sys/types.h>
#include <yaosp/ipc.h>
#include <yaosp/debug.h>
#include <yaosp/semaphore.h>

#include <ygui/protocol.h>

ipc_port_id guiserver_port = -1;

ipc_port_id app_reply_port = -1;
ipc_port_id app_client_port = -1;
ipc_port_id app_server_port = -1;

semaphore_id app_lock = -1;

int create_application( void ) {
    int error;
    struct timespec slp_time;

    msg_create_app_t request;
    msg_create_app_reply_t reply;

    /* Get the guiserver port ... */

    dbprintf( "Waiting for guiserver to start ...\n" );

    while ( 1 ) {
        error = get_named_ipc_port( "guiserver", &guiserver_port );

        if ( error == 0 ) {
            break;
        }

        slp_time.tv_sec = 1;
        slp_time.tv_nsec = 0;

        nanosleep( &slp_time, NULL );
    }

    dbprintf( "Guiserver port: %d\n", guiserver_port );

    /* Register our own application */

    app_client_port = create_ipc_port();

    if ( app_client_port < 0 ) {
        return app_client_port;
    }

    app_reply_port = create_ipc_port();

    if ( app_reply_port < 0 ) {
        return app_reply_port;
    }

    app_lock = create_semaphore( "application lock", SEMAPHORE_BINARY, 0, 1 );

    if ( app_lock < 0 ) {
        return app_lock;
    }

    request.reply_port = app_reply_port;
    request.client_port = app_client_port;

    error = send_ipc_message( guiserver_port, MSG_CREATE_APPLICATION, &request, sizeof( msg_create_app_t ) );

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

int run_application( void ) {
    int error;
    uint32_t code;
    void* buffer;

    if ( ( guiserver_port == -1 ) ||
         ( app_client_port == -1 ) ||
         ( app_reply_port == -1 ) ||
         ( app_server_port == -1 ) ) {
        return -EINVAL;
    }

    buffer = malloc( MAX_APPLICATION_BUFSIZE );

    if ( buffer == NULL ) {
        return -ENOMEM;
    }

    dbprintf( "Application running ...\n" );

    while ( 1 ) {
        error = recv_ipc_message( app_client_port, &code, buffer, MAX_APPLICATION_BUFSIZE, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            dbprintf( "run_application(): Failed to receive message: %d\n", error );
            break;
        }

        switch ( code ) {
            default :
                dbprintf( "run_application(): Received unknown message: %x\n", code );
                break;
        }
    }

    return 0;
}
