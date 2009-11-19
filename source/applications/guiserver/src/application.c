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
#include <windowmanager.h>

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

typedef struct {
    size_t size;
    uint8_t* data;
} window_list_t;

static int window_register_helper( window_t* window, void* data ) {
    int title_len;
    size_t new_size;
    uint8_t* new_data;
    msg_win_info_t* win_info;
    window_list_t* list = ( window_list_t* )data;

    if ( window->flags & WINDOW_NO_BORDER ) {
        return 0;
    }

    *( int* )list->data += 1;

    title_len = strlen( window->title );
    new_size = list->size + sizeof( msg_win_info_t ) + title_len + 1;

    new_data = ( uint8_t* )realloc( list->data, new_size );

    if ( new_data == NULL ) {
        return -ENOMEM;
    }

    win_info = ( msg_win_info_t* )( new_data + list->size );

    win_info->id = window->id;
    memcpy( win_info + 1, window->title, title_len + 1 );

    list->size = new_size;
    list->data = new_data;

    return 0;
}

static int handle_reg_window_listener( application_t* app, int get_window_list ) {
    window_list_t list;

    if ( get_window_list ) {
        list.size = sizeof( int );
        list.data = ( uint8_t* )malloc( list.size );

        if ( list.data == NULL ) {
            return -ENOMEM;
        }

        *( int* )list.data = 0;
    }

    pthread_mutex_lock( &wm_lock );

    /* Send the current window list to the application */

    if ( get_window_list ) {
        wm_iterate_window_list( window_register_helper, ( void* )&list );
        send_ipc_message( app->client_port, MSG_WINDOW_LIST, ( void* )list.data, list.size );
        free( list.data );
    }

    /* Register the application as a window listener */

    wm_add_window_listener( app );

    pthread_mutex_unlock( &wm_lock );

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

        if ( error < 0 ) {
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

            case MSG_REG_WINDOW_LISTENER :
                handle_reg_window_listener( app, *( int* )buffer );
                break;

            case MSG_WINDOW_BRING_TO_FRONT :
                wm_bring_to_front_by_id( *( int* )buffer );
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
