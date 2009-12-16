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
#include <pthread.h>
#include <yaosp/debug.h>

#include <window.h>
#include <windowdecorator.h>
#include <windowmanager.h>
#include <graphicsdriver.h>
#include <mouse.h>

#define MAX_WINDOW_BUFSIZE 33000

extern window_decorator_t* window_decorator;

static window_t* window_create( const char* title, uint32_t flags ) {
    window_t* window;

    window = ( window_t* )calloc( 1, sizeof( window_t ) );

    if ( window == NULL ) {
        goto error1;
    }

    window->title = strdup( title );

    if ( window->title == NULL ) {
        goto error2;
    }

    window->server_port = create_ipc_port();

    if ( window->server_port < 0 ) {
        goto error3;
    }

    if ( init_region( &window->visible_regions ) != 0 ) {
        goto error4;
    }

    window->id = -1;
    window->flags = flags;

    return window;

 error4:
    destroy_ipc_port( window->server_port );

 error3:
    free( window->title );

 error2:
    free( window );

 error1:
    return NULL;
}

static int window_destroy( window_t* window ) {
    if ( window->icon != NULL ) {
        bitmap_put( window->icon );
    }

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        window_decorator->destroy( window );
    }

    if ( window->bitmap != NULL ) {
        bitmap_put( window->bitmap );
        window->bitmap = NULL;
    }

    destroy_region( &window->visible_regions );
    destroy_ipc_port( window->server_port );
    free( window->title );
    free( window );

    return 0;
}

static void window_do_resize( window_t* window, msg_win_do_resize_t* request ) {
    int width;
    int height;
    msg_win_resized_t reply;

    if ( ( request->size.x == 0 ) ||
         ( request->size.y == 0 ) ) {
        dbprintf(
            "window_do_resize(): Invalid resize request! (x=%d, y=%d)\n",
            request->size.x,
            request->size.y
        );
    }

    /* Lock the window manager if the window is visible */

    if ( window->is_visible ) {
        pthread_mutex_lock( &wm_lock );
    }

    /* Put the current bitmap of the window */

    if ( window->bitmap != NULL ) {
        bitmap_put( window->bitmap );
    }

    if ( window->flags & WINDOW_NO_BORDER ) {
        rect_init(
            &window->screen_rect,
            window->screen_rect.left,
            window->screen_rect.top,
            window->screen_rect.left + request->size.x - 1,
            window->screen_rect.top + request->size.y - 1
        );

        rect_copy( &window->client_rect, &window->screen_rect );
    } else {
        dbprintf( "window_do_resize(): Resizing a window with border not yet implemented ...\n" );
    }

    rect_bounds( &window->screen_rect, &width, &height );

    window->bitmap = create_bitmap( width, height, CS_RGB32 );

    if ( window->bitmap == NULL ) {
        dbprintf( "window_do_resize(): No memory for new window bitmap!\n" );
        /* TODO: handle the error properly */
    }

    if ( window->is_visible ) {
        wm_window_resized( window );

        pthread_mutex_unlock( &wm_lock );
    }

    point_copy( &reply.size, &request->size );

    send_ipc_message( request->reply_port, MSG_WINDOW_RESIZED, &reply, sizeof( msg_win_resized_t ) );
}

static void window_do_move( window_t* window, msg_win_do_move_t* request ) {
    msg_win_moved_t reply;

    /* Lock the window manager if the window is visible */

    if ( window->is_visible ) {
        pthread_mutex_lock( &wm_lock );
    }

    if ( window->flags & WINDOW_NO_BORDER ) {
        rect_init(
            &window->screen_rect,
            request->position.x,
            request->position.y,
            request->position.x + window->bitmap->width - 1,
            request->position.y + window->bitmap->height - 1
        );

        rect_copy( &window->client_rect, &window->screen_rect );
    } else {
        dbprintf( "window_do_move(): Moving a window with border not yet implemented ...\n" );
    }

    if ( window->is_visible ) {
        wm_window_moved( window );

        pthread_mutex_unlock( &wm_lock );
    }

    point_copy( &reply.position, &request->position );

    send_ipc_message( request->reply_port, MSG_WINDOW_MOVED, &reply, sizeof( msg_win_moved_t ) );
}

static int window_set_icon( window_t* window, msg_win_set_icon_t* request ) {
    window->icon = bitmap_get( request->icon_bitmap );

    send_ipc_message( request->reply_port, MSG_WINDOW_ICON_UPDATED, NULL, 0 );

    return 0;
}

static void* window_thread( void* arg ) {
    int error;
    int running;
    uint32_t code;
    void* buffer;
    window_t* window;

    window = ( window_t* )arg;

    buffer = malloc( MAX_WINDOW_BUFSIZE );

    if ( buffer == NULL ) {
        return NULL;
    }

    running = 1;

    while ( running ) {
        error = recv_ipc_message( window->server_port, &code, buffer, MAX_WINDOW_BUFSIZE, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            dbprintf( "window_thread(): Failed to receive message: %d\n", error );
            continue;
        }

        switch ( code ) {
            case MSG_RENDER_COMMANDS :
                window_do_render( window, ( uint8_t* )buffer, error );
                break;

            case MSG_WINDOW_SHOW :
                wm_register_window( window );
                window->is_visible = 1;
                break;

            case MSG_WINDOW_HIDE :
                wm_unregister_window( window );
                window->is_visible = 0;
                break;

            case MSG_WINDOW_DESTROY :
                wm_unregister_window( window );
                running = 0;
                break;

            case MSG_WINDOW_DO_RESIZE :
                window_do_resize( window, ( msg_win_do_resize_t* )buffer );
                break;

            case MSG_WINDOW_DO_MOVE :
                window_do_move( window, ( msg_win_do_move_t* )buffer );
                break;

            case MSG_WINDOW_SET_ICON :
                window_set_icon( window, ( msg_win_set_icon_t* )buffer );
                break;

            default :
                dbprintf( "window_thread(): Received unknown message: %x\n", code );
                break;
        }
    }

    application_remove_window( window->application, window );

    window_destroy( window );
    free( buffer );

    return NULL;
}

int handle_create_window( application_t* application, msg_create_win_t* request ) {
    int error;
    int width;
    int height;
    window_t* window;
    pthread_t thread;
    pthread_attr_t attrib;
    msg_create_win_reply_t reply;

    window = window_create( ( const char* )( request + 1 ), request->flags );

    if ( window == NULL ) {
        goto error1;
    }

    window->client_port = request->client_port;
    window->is_visible = 0;
    window->is_moving = 0;
    window->mouse_on_decorator = 0;
    window->drawing_mode = DM_COPY;
    window->application = application;

    /* Initialize rendering stuffs */

    window->font = NULL;

    if ( window->flags & WINDOW_NO_BORDER ) {
        rect_init(
            &window->screen_rect,
            request->position.x,
            request->position.y,
            request->position.x + request->size.x - 1,
            request->position.y + request->size.y - 1
        );

        rect_copy( &window->client_rect, &window->screen_rect );
    } else {
        rect_init(
            &window->screen_rect,
            request->position.x,
            request->position.y,
            request->position.x + request->size.x + window_decorator->border_size.x - 1,
            request->position.y + request->size.y + window_decorator->border_size.y - 1
        );

        rect_resize_n(
            &window->client_rect,
            &window->screen_rect,
            window_decorator->lefttop_offset.x,
            window_decorator->lefttop_offset.y,
            window_decorator->lefttop_offset.x,
            window_decorator->lefttop_offset.y
        );
    }

    if ( rect_is_valid( &window->screen_rect ) ) {
        rect_bounds( &window->screen_rect, &width, &height );

        window->bitmap = create_bitmap( width, height, CS_RGB32 );

        if ( window->bitmap == NULL ) {
            goto error5;
        }
    } else {
        window->bitmap = NULL;
    }

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        error = window_decorator->initialize( window );

        if ( error < 0 ) {
            goto error5;
        }

        window_decorator->calculate_regions( window );
    }

    pthread_attr_init( &attrib );
    pthread_attr_setname( &attrib, "win_event" );

    error = pthread_create(
        &thread,
        &attrib,
        window_thread,
        ( void* )window
    );

    pthread_attr_destroy( &attrib );

    if ( error != 0 ) {
        goto error5;
    }

    reply.server_port = window->server_port;

    /* Insert the new window into the application's window list */

    application_insert_window( application, window );

    goto out;

error5:
    window_destroy( window );

error1:
    reply.server_port = -1;

out:
    send_ipc_message( request->reply_port, 0, &reply, sizeof( msg_create_win_reply_t ) );

    return 0;
}

int window_activated( window_t* window ) {
    send_ipc_message( window->client_port, MSG_WINDOW_ACTIVATED, NULL, 0 );

    return 0;
}

int window_deactivated( window_t* window ) {
    send_ipc_message( window->client_port, MSG_WINDOW_DEACTIVATED, NULL, 0 );

    return 0;
}

int window_moved( window_t* window ) {
    msg_win_moved_t msg;

    rect_lefttop( &window->screen_rect, &msg.position );

    send_ipc_message( window->client_port, MSG_WINDOW_MOVED, &msg, sizeof( msg_win_moved_t ) );

    return 0;
}

int window_key_pressed( window_t* window, int key ) {
    msg_key_pressed_t cmd;

    cmd.key = key;

    send_ipc_message( window->client_port, MSG_KEY_PRESSED, &cmd, sizeof( msg_key_pressed_t ) );

    return 0;
}

int window_key_released( window_t* window, int key ) {
    msg_key_released_t cmd;

    cmd.key = key;

    send_ipc_message( window->client_port, MSG_KEY_RELEASED, &cmd, sizeof( msg_key_released_t ) );

    return 0;
}

int window_mouse_entered( window_t* window, point_t* mouse_position ) {
    point_t window_position;

    point_sub_xy_n( &window_position, mouse_position, window->screen_rect.left, window->screen_rect.top );

    window->mouse_on_decorator = \
        ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) &&
        ( window_decorator->border_has_position( window, mouse_position ) );

    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_entered( window, mouse_position );
    } else {
        msg_mouse_entered_t cmd;

        point_sub_xy_n(
            &cmd.mouse_position,
            mouse_position,
            window->client_rect.left,
            window->client_rect.top
        );

        send_ipc_message( window->client_port, MSG_MOUSE_ENTERED, &cmd, sizeof( msg_mouse_entered_t ) );
    }

    return 0;
}

int window_mouse_exited( window_t* window ) {
    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_exited( window );
    } else {
        send_ipc_message( window->client_port, MSG_MOUSE_EXITED, NULL, 0 );
    }

    return 0;
}

int window_mouse_moved( window_t* window, point_t* mouse_position ) {
    int on_decorator;

    on_decorator = \
        ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) &&
        ( window_decorator->border_has_position( window, mouse_position ) );

    if ( on_decorator ) {
        if ( window->mouse_on_decorator ) {
            window_decorator->mouse_moved( window, mouse_position );
        } else {
            window_decorator->mouse_entered( window, mouse_position );

            send_ipc_message( window->client_port, MSG_MOUSE_EXITED, NULL, 0 );
        }
    } else {
        if ( window->mouse_on_decorator ) {
            msg_mouse_entered_t cmd;

            window_decorator->mouse_exited( window );

            point_sub_xy_n(
                &cmd.mouse_position,
                mouse_position,
                window->client_rect.left,
                window->client_rect.top
            );

            send_ipc_message( window->client_port, MSG_MOUSE_ENTERED, &cmd, sizeof( msg_mouse_entered_t ) );
        } else {
            msg_mouse_moved_t cmd;

            point_sub_xy_n(
                &cmd.mouse_position,
                mouse_position,
                window->client_rect.left,
                window->client_rect.top
            );

            send_ipc_message( window->client_port, MSG_MOUSE_MOVED, &cmd, sizeof( msg_mouse_moved_t ) );
        }
    }

    window->mouse_on_decorator = on_decorator;

    return 0;
}

int window_mouse_pressed( window_t* window, int mouse_button ) {
    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_pressed( window, mouse_button );
    } else {
        msg_mouse_pressed_t cmd;

        mouse_get_position( &cmd.mouse_position );
        point_sub_xy( &cmd.mouse_position, window->client_rect.left, window->client_rect.top );

        cmd.button = mouse_button;

        send_ipc_message( window->client_port, MSG_MOUSE_PRESSED, &cmd, sizeof( msg_mouse_pressed_t ) );
    }

    window->mouse_pressed_on_decorator = window->mouse_on_decorator;

    return 0;
}

int window_mouse_released( window_t* window, int mouse_button ) {
    if ( window->mouse_pressed_on_decorator ) {
        window_decorator->mouse_released( window, mouse_button );
    } else {
        msg_mouse_released_t cmd;

        cmd.button = mouse_button;

        send_ipc_message( window->client_port, MSG_MOUSE_RELEASED, &cmd, sizeof( msg_mouse_released_t ) );
    }

    return 0;
}

int window_close_request( window_t* window ) {
    send_ipc_message( window->client_port, MSG_WINDOW_CLOSE_REQUEST, NULL, 0 );

    return 0;
}
