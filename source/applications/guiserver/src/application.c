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
#include <errno.h>
#include <pthread.h>
#include <yaosp/debug.h>

#include <ygui/protocol.h>

#include <application.h>
#include <window.h>
#include <fontmanager.h>
#include <graphicsdriver.h>
#include <bitmap.h>

#define MAX_APPLICATION_BUFSIZE 512

static int handle_create_font( msg_create_font_t* request ) {
    char* family;
    char* style;
    font_node_t* font;
    msg_create_font_reply_t reply;

    family = ( char* )( request + 1 );
    style = family + strlen( family ) + 1;

    font = font_manager_get( family, style, &request->properties );

    if ( font == NULL ) {
        reply.handle = -1;
    } else {
        reply.handle = ( int )font;
        reply.ascender = font->ascender;
        reply.descender = font->descender;
        reply.line_gap = font->line_gap;
    }

    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_font_reply_t ) );

    return 0;
}

static int handle_get_string_width( msg_get_str_width_t* request ) {
    font_node_t* font;
    msg_get_str_width_reply_t reply;

    font = ( font_node_t* )request->font_handle;

    reply.width = font_node_get_string_width( font, ( const char* )( request + 1 ), request->length );

    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_get_str_width_reply_t ) );

    return 0;
}

static int handle_get_desktop_size( msg_desk_get_size_t* request ) {
    msg_desk_get_size_reply_t reply;

    point_init(
        &reply.size,
        rect_width( &screen_rect ),
        rect_height( &screen_rect )
    );

    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_desk_get_size_reply_t ) );

    return 0;
}

static void* application_thread( void* arg ) {
    int error;
    void* buffer;
    uint32_t code;
    application_t* app;

    app = ( application_t* )arg;

    buffer = malloc( MAX_APPLICATION_BUFSIZE );

    if ( buffer == NULL ) {
        return NULL;
    }

    while ( 1 ) {
        error = recv_ipc_message( app->server_port, &code, buffer, MAX_APPLICATION_BUFSIZE, INFINITE_TIMEOUT );

        if ( error == -ENOENT ) {
            dbprintf( "application_thread(): Skipping -ENOENT ...\n" );
            continue;
        } else if ( error < 0 ) {
            dbprintf( "application_thread(): Failed to receive message: %d\n", error );
            break;
        }

        switch ( code ) {
            case MSG_WINDOW_CREATE :
                handle_create_window( ( msg_create_win_t* )buffer );
                break;

            case MSG_FONT_CREATE :
                handle_create_font( ( msg_create_font_t* )buffer );
                break;

            case MSG_FONT_GET_STR_WIDTH :
                handle_get_string_width( ( msg_get_str_width_t* )buffer );
                break;

            case MSG_BITMAP_CREATE :
                handle_create_bitmap( ( msg_create_bitmap_t* )buffer );
                break;

            case MSG_DESK_GET_SIZE :
                handle_get_desktop_size( ( msg_desk_get_size_t* )buffer );
                break;

            default :
                dbprintf( "application_thread(): Received unknown message: %x\n", code );
                break;
        }
    }

    return NULL;
}

int handle_create_application( msg_create_app_t* request ) {
    int error;
    application_t* app;
    pthread_t app_thread;
    pthread_attr_t attrib;
    msg_create_app_reply_t reply;

    app = ( application_t* )malloc( sizeof( application_t ) );

    if ( app == NULL ) {
        goto error1;
    }

    app->server_port = create_ipc_port();

    if ( app->server_port < 0 ) {
        goto error2;
    }

    app->client_port = request->client_port;

    pthread_attr_init( &attrib );
    pthread_attr_setname( &attrib, "app_event" );

    error = pthread_create(
        &app_thread,
        &attrib,
        application_thread,
        ( void* )app
    );

    pthread_attr_destroy( &attrib );

    if ( error != 0 ) {
        goto error3;
    }

    reply.server_port = app->server_port;

    goto out;

error3:
    destroy_ipc_port( app->server_port );

error2:
    free( app );

error1:
    reply.server_port = -1;

out:
    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_app_reply_t ) );

    return 0;
}
