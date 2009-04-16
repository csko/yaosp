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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <yaosp/thread.h>
#include <yaosp/debug.h>

#include <ygui/window.h>
#include <ygui/protocol.h>
#include <ygui/panel.h>
#include <ygui/render/render.h>

#include "internal.h"

extern ipc_port_id app_server_port;

#define MAX_WINDOW_BUFSIZE 512

widget_t* window_get_container( window_t* window ) {
    return window->container;
}

static int window_thread( void* arg ) {
    int error;
    void* buffer;
    uint32_t code;
    window_t* window;

    window = ( window_t* )arg;

    buffer = malloc( MAX_WINDOW_BUFSIZE );

    if ( buffer == NULL ) {
        return -ENOMEM;
    }

    while ( 1 ) {
        error = recv_ipc_message( window->client_port, &code, buffer, MAX_WINDOW_BUFSIZE, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            dbprintf( "window_thread(): Failed to receive message: %x\n", error );
            break;
        }

        switch ( code ) {
            case MSG_DO_SHOW_WINDOW :
                /* Paint all the widgets */

                widget_paint( window->container );

                /* Send the rendering commands to the guiserver */

                flush_render_buffer( window );

                /* Show the window */

                send_ipc_message( window->server_port, MSG_SHOW_WINDOW, NULL, 0 );

                break;

            default :
                dbprintf( "window_thread(): Received unknown message: %x\n", code );
                break;
        }
    }

    return 0;
}

window_t* create_window( const char* title, point_t* position, point_t* size, int flags ) {
    int error;
    window_t* window;
    size_t title_size;
    msg_create_win_t* request;
    msg_create_win_reply_t reply;

    thread_id thread;

    /* Do some sanity checking */

    if ( ( title == NULL ) ||
         ( position == NULL ) ||
         ( size == NULL ) ) {
        goto error1;
    }

    title_size = strlen( title );

    /* Create the window object */

    window = ( window_t* )malloc( sizeof( window_t ) );

    if ( window == NULL ) {
        goto error1;
    }

    window->reply_port = create_ipc_port();

    if ( window->reply_port < 0 ) {
        goto error2;
    }

    window->client_port = create_ipc_port();

    if ( window->reply_port < 0 ) {
        goto error3;
    }

    window->container = create_panel();

    if ( window->container == NULL ) {
        goto error4;
    }

    widget_set_window( window->container, window );

    point_init( &window->container->position, 0, 0 );
    point_init( &window->container->size, size->x, size->y );

    window->render_buffer = ( uint8_t* )malloc( DEFAULT_RENDER_BUFFER_SIZE );

    if ( window->render_buffer == NULL ) {
        goto error5;
    }

    initialize_render_buffer( window );
    window->render_buffer_max_size = DEFAULT_RENDER_BUFFER_SIZE;

    /* Register the window to the guiserver */

    request = ( msg_create_win_t* )malloc( sizeof( msg_create_win_t ) + title_size + 1 );

    if ( request == NULL ) {
        goto error5;
    }

    request->reply_port = window->reply_port;
    request->client_port = window->client_port;
    memcpy( &request->position, position, sizeof( point_t ) );
    memcpy( &request->size, size, sizeof( point_t ) );
    memcpy( ( void* )( request + 1 ), title, title_size + 1 );
    request->flags = flags;

    error = send_ipc_message( app_server_port, MSG_CREATE_WINDOW, request, sizeof( msg_create_win_t ) + title_size + 1 );

    free( request );

    if ( error < 0 ) {
        goto error5;
    }

    error = recv_ipc_message( window->reply_port, NULL, &reply, sizeof( msg_create_win_reply_t ), INFINITE_TIMEOUT );

    if ( error < 0 ) {
        goto error5;
    }

    if ( reply.server_port < 0 ) {
        goto error5;
    }

    window->server_port = reply.server_port;

    thread = create_thread(
        "window",
        PRIORITY_DISPLAY,
        window_thread,
        ( void* )window,
        0
    );

    if ( thread < 0 ) {
        goto error6;
    }

    wake_up_thread( thread );

    return window;

error6:
    /* TODO: Unregister the window from the guiserver */

error5:
    /* TODO: free the container */

error4:
    /* TODO: Delete the client port */

error3:
    /* TODO: Delete the reply port */

error2:
    free( window );

error1:
    return NULL;
}

int show_window( window_t* window ) {
    int error;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = send_ipc_message( window->client_port, MSG_DO_SHOW_WINDOW, NULL, 0 );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int hide_window( window_t* window ) {
    int error;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = send_ipc_message( window->client_port, MSG_DO_HIDE_WINDOW, NULL, 0 );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
