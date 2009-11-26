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

static application_t* application_create( ipc_port_id client_port ) {
    application_t* app;

    app = ( application_t* )malloc( sizeof( application_t ) );

    if ( app == NULL ) {
        goto error1;
    }

    app->server_port = create_ipc_port();

    if ( app->server_port < 0 ) {
        goto error2;
    }

    if ( init_array( &app->window_list ) != 0 ) {
        goto error3;
    }

    if ( init_array( &app->bitmap_list ) != 0 ) {
        goto error4;
    }

    if ( init_array( &app->font_list ) != 0 ) {
        goto error5;
    }

    if ( pthread_mutex_init( &app->lock, NULL ) != 0 ) {
        goto error6;
    }

    app->client_port = client_port;

    return app;

 error6:
    destroy_array( &app->font_list );

 error5:
    destroy_array( &app->bitmap_list );

 error4:
    destroy_array( &app->window_list );

 error3:
    destroy_ipc_port( app->server_port );

 error2:
    free( app );

 error1:
    return NULL;
}

static int application_destroy( application_t* app ) {
    int size;

    size = array_get_size( &app->bitmap_list );

    if ( size > 0 ) {
        int i;

        dbprintf( "Application forgot to delete %d bitmaps.\n", size );

        for ( i = 0; i < size; i++ ) {
            bitmap_t* bitmap = ( bitmap_t* )array_get_item( &app->bitmap_list, i );

            bitmap_put( bitmap );
        }
    }

    pthread_mutex_destroy( &app->lock );
    destroy_array( &app->font_list );
    destroy_array( &app->bitmap_list );
    destroy_array( &app->window_list );
    destroy_ipc_port( app->server_port );
    free( app );

    return 0;
}

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

    if ( window->icon == NULL ) {
        win_info->icon_bitmap = -1;
    } else {
        win_info->icon_bitmap = window->icon->id;
    }

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
    int running;
    void* buffer;
    uint32_t code;
    application_t* app;

    app = ( application_t* )arg;

    buffer = malloc( MAX_APPLICATION_BUFSIZE );

    if ( buffer == NULL ) {
        return NULL;
    }

    running = 1;

    while ( running ) {
        error = recv_ipc_message( app->server_port, &code, buffer, MAX_APPLICATION_BUFSIZE, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            dbprintf( "application_thread(): Failed to receive message: %d\n", error );
            break;
        }

        switch ( code ) {
            case MSG_APPLICATION_DESTROY :
                running = 0;
                break;

            case MSG_WINDOW_CREATE :
                handle_create_window( app, ( msg_create_win_t* )buffer );
                break;

            case MSG_FONT_CREATE :
                handle_create_font( ( msg_create_font_t* )buffer );
                break;

            case MSG_FONT_GET_STR_WIDTH :
                handle_get_string_width( ( msg_get_str_width_t* )buffer );
                break;

            case MSG_BITMAP_CREATE :
                handle_create_bitmap( app, ( msg_create_bitmap_t* )buffer );
                break;

            case MSG_BITMAP_CLONE :
                handle_clone_bitmap( app, ( msg_clone_bitmap_t* )buffer );
                break;

            case MSG_BITMAP_DELETE :
                handle_delete_bitmap( app, ( msg_delete_bitmap_t* )buffer );
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

    application_destroy( app );

    return NULL;
}

int application_insert_bitmap( application_t* application, bitmap_t* bitmap ) {
    pthread_mutex_lock( &application->lock );
    array_add_item( &application->bitmap_list, ( void* )bitmap );
    pthread_mutex_unlock( &application->lock );

    return 0;
}

int application_remove_bitmap( application_t* application, bitmap_t* bitmap ) {
    int error;

    pthread_mutex_lock( &application->lock );
    error = array_remove_item( &application->bitmap_list, ( void* )bitmap );
    pthread_mutex_unlock( &application->lock );

    if ( error < 0 ) {
        dbprintf( "application_remove_bitmap(): Tried to remove invalid bitmap!\n" );
    }

    return 0;
}

int application_insert_window( application_t* application, window_t* window ) {
    pthread_mutex_lock( &application->lock );
    array_add_item( &application->window_list, ( void* )window );
    pthread_mutex_unlock( &application->lock );

    return 0;
}

int application_remove_window( application_t* application, window_t* window ) {
    pthread_mutex_lock( &application->lock );

    array_remove_item( &application->window_list, ( void* )window );

    if ( array_get_size( &application->window_list ) == 0 ) {
        send_ipc_message( application->client_port, MSG_APPLICATION_DESTROY, NULL, 0 );
    }

    pthread_mutex_unlock( &application->lock );

    return 0;
}

int handle_create_application( msg_create_app_t* request ) {
    int error;
    application_t* app;
    pthread_t app_thread;
    pthread_attr_t attrib;
    msg_create_app_reply_t reply;

    app = application_create( request->client_port );

    if ( app == NULL ) {
        goto error1;
    }

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
        goto error2;
    }

    reply.server_port = app->server_port;

    goto out;

 error2:
    application_destroy( app );

 error1:
    reply.server_port = -1;

 out:
    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_app_reply_t ) );

    return 0;
}
