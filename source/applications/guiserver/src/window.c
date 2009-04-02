/* GUI server
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <yaosp/thread.h>
#include <yaosp/debug.h>

#include <window.h>

#define MAX_WINDOW_BUFSIZE 512

static int window_thread( void* arg ) {
    int error;
    uint32_t code;
    void* buffer;
    window_t* window;

    window = ( window_t* )arg;

    buffer = malloc( MAX_WINDOW_BUFSIZE );

    if ( buffer == NULL ) {
        return -ENOMEM;
    }

    while ( 1 ) {
        error = recv_ipc_message( window->server_port, &code, buffer, MAX_WINDOW_BUFSIZE, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            dbprintf( "window_thread(): Failed to receive message: %d\n", error );
            break;
        }

        switch ( code ) {
            case MSG_SHOW_WINDOW :
                break;

            default :
                dbprintf( "window_thread(): Received unknown message: %x\n", code );
                break;
        }
    }

    free( buffer );

    return 0;
}

int handle_create_window( msg_create_win_t* request ) {
    window_t* window;
    thread_id thread;
    msg_create_win_reply_t reply;

    window = ( window_t* )malloc( sizeof( window_t ) );

    if ( window == NULL ) {
        goto error1;
    }

    window->title = strdup( ( const char* )( request + 1 ) );

    if ( window->title == NULL ) {
        goto error2;
    }

    window->server_port = create_ipc_port();

    if ( window->server_port < 0 ) {
        goto error3;
    }

    window->client_port = request->client_port;
    memcpy( &window->position, &request->position, sizeof( point_t ) );
    memcpy( &window->size, &request->size, sizeof( point_t ) );
    window->flags = request->flags;

    thread = create_thread(
        "window",
        PRIORITY_DISPLAY,
        window_thread,
        window,
        0
    );

    if ( thread < 0 ) {
        goto error4;
    }

    wake_up_thread( thread );

    reply.server_port = window->server_port;

    goto out;

error4:
    /* TODO: Delete server port */

error3:
    free( window->title );

error2:
    free( window );

error1:
    reply.server_port = -1;

out:
    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_win_reply_t ) );

    return 0;
}
