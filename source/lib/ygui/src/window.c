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
#include <assert.h>
#include <unistd.h>
#include <yaosp/debug.h>

#include <ygui/window.h>
#include <ygui/protocol.h>
#include <ygui/panel.h>
#include <ygui/render/render.h>

#include "internal.h"

extern ipc_port_id app_server_port;

#define MAX_WINDOW_BUFSIZE 256

static window_t* window_create( point_t* size ) {
    window_t* window;

    window = ( window_t* )calloc( 1, sizeof( window_t ) );

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

    /* Initialize render buffer */

    window->render_buffer = ( uint8_t* )malloc( DEFAULT_RENDER_BUFFER_SIZE );

    if ( window->render_buffer == NULL ) {
        goto error4;
    }

    initialize_render_buffer( window );
    window->render_buffer_max_size = DEFAULT_RENDER_BUFFER_SIZE;

    /* Create the root container */

    window->container = create_panel();

    if ( window->container == NULL ) {
        goto error5;
    }

    widget_set_window( window->container, window );

    if ( size != NULL ) {
        point_t container_position = { .x = 0, .y = 0 };

        widget_set_position_and_size(
            window->container,
            &container_position,
            size
        );
    }

    /* Initialize the graphics context of the window */

    if ( init_gc( window, &window->gc ) != 0 ) {
        goto error6;
    }

    return window;

 error6:
    widget_dec_ref( window->container );

 error5:
    free( window->render_buffer );

 error4:
    destroy_ipc_port( window->client_port );

 error3:
    destroy_ipc_port( window->reply_port );

 error2:
    free( window );

 error1:
    return NULL;
}

static int window_destroy( window_t* window ) {
    destroy_gc( &window->gc );
    widget_dec_ref( window->container );
    free( window->render_buffer );
    destroy_ipc_port( window->client_port );
    destroy_ipc_port( window->reply_port );
    free( window );

    return 0;
}

widget_t* window_get_container( window_t* window ) {
    return window->container;
}

static widget_t* window_find_widget_at_helper( widget_t* widget, point_t* position,
                                               point_t* lefttop, rect_t* visible_rect ) {
    int i;
    int count;
    rect_t widget_rect;

    rect_init(
        &widget_rect,
        0,
        0,
        widget->full_size.x - 1,
        widget->full_size.y - 1
    );

    rect_add_point( &widget_rect, lefttop );

    rect_and( &widget_rect, visible_rect );

    if ( ( !rect_is_valid( &widget_rect ) ) ||
         ( !rect_has_point( &widget_rect, position ) ) ) {
        return NULL;
    }

    count = widget_get_child_count( widget );

    for ( i = 0; i < count; i++ ) {
        widget_t* child;
        widget_t* result;
        rect_t new_visible_rect;

        child = widget_get_child_at( widget, i );

        point_add( lefttop, &child->position );
        point_add( lefttop, &child->scroll_offset );

        rect_resize_n(
            &new_visible_rect,
            visible_rect,
            child->position.x,
            child->position.y,
            0,
            0
        );

        new_visible_rect.right = MIN(
            new_visible_rect.right,
            new_visible_rect.left + child->visible_size.x - 1
        );

        new_visible_rect.bottom = MIN(
            new_visible_rect.bottom,
            new_visible_rect.top + child->visible_size.y - 1
        );

        result = window_find_widget_at_helper( child, position, lefttop, &new_visible_rect );

        if ( result != NULL ) {
            return result;
        }

        point_sub( lefttop, &child->scroll_offset );
        point_sub( lefttop, &child->position );
    }

    return widget;
}

static widget_t* window_find_widget_at( window_t* window, point_t* position ) {
    point_t lefttop = {
        .x = 0,
        .y = 0
    };
    rect_t visible_rect = {
        .left = 0,
        .top = 0,
        .right = window->container->visible_size.x - 1,
        .bottom = window->container->visible_size.y - 1
    };

    return window_find_widget_at_helper( window->container, position, &lefttop, &visible_rect );
}

static void window_signal_event_handler( window_t* window, int event ) {
    window_event_data_t* event_data;

    assert( ( event >= 0 ) && ( event < WE_COUNT ) );

    event_data = &window->event_handlers[ event ];

    if ( event_data->callback != NULL ) {
        event_data->callback( window, event_data->data );
    }
}

static void* window_thread( void* arg ) {
    int error;
    int running;
    void* buffer;
    uint32_t code;
    window_t* window;

    window = ( window_t* )arg;

    buffer = malloc( MAX_WINDOW_BUFSIZE );

    if ( buffer == NULL ) {
        return NULL;
    }

    running = 1;

    while ( running ) {
        error = recv_ipc_message( window->client_port, &code, buffer, MAX_WINDOW_BUFSIZE, INFINITE_TIMEOUT );

        if ( error == -ENOENT ) {
            dbprintf( "window_thread(): Received ENOENT, skipping ...\n" );
            continue;
        }

        if ( error < 0 ) {
            dbprintf( "window_thread(): Failed to receive message: %d\n", error );
            break;
        }

        switch ( code ) {
            case MSG_WINDOW_DO_SHOW :
            case MSG_WIDGET_INVALIDATED : {
                int error;
                rect_t res_area;
                render_header_t* header;

                /* Setup the initial restricted area */

                rect_init(
                    &res_area,
                    0,
                    0,
                    window->container->visible_size.x - 1,
                    window->container->visible_size.y - 1
                );

                gc_push_restricted_area( &window->gc, &res_area );

                /* Re-paint the widgets */

                widget_paint( window->container, &window->gc );

                /* Clean-up the graphics context */

                gc_clean_up( &window->gc );

                /* Add the terminator item to the render command list */

                error = allocate_render_packet( window, sizeof( render_header_t ), ( void** )&header );

                if ( error >= 0 ) {
                    header->command = R_DONE;
                }

                /* Send the rendering commands to the guiserver */

                error = flush_render_buffer( window );

                if ( error < 0 ) {
                    dbprintf( "window_thread(): Failed to flush render buffer: %d.\n", error );
                }

                /* Show the window (if needed) */

                if ( code == MSG_WINDOW_DO_SHOW ) {
                    send_ipc_message( window->server_port, MSG_WINDOW_SHOW, NULL, 0 );
                }

                break;
            }

            case MSG_WINDOW_DO_HIDE :
                send_ipc_message( window->server_port, MSG_WINDOW_HIDE, NULL, 0 );
                break;

            case MSG_WINDOW_DO_RESIZE : {
                int error;
                point_t position = { .x = 0, .y = 0 };
                msg_win_resized_t reply;

                error = send_ipc_message(
                    window->server_port,
                    MSG_WINDOW_DO_RESIZE,
                    buffer,
                    sizeof( msg_win_do_resize_t )
                );

                if ( error < 0 ) {
                    dbprintf( "window_thread(): Failed to send resize request!\n" );
                    break;
                }

                error = recv_ipc_message(
                    window->reply_port,
                    NULL,
                    &reply,
                    sizeof( msg_win_resized_t ),
                    INFINITE_TIMEOUT
                );

                if ( error < 0 ) {
                    dbprintf( "window_thread(): Failed to get resize reply!\n" );
                    break;
                }

                widget_set_position_and_size(
                    window->container,
                    &position,
                    &reply.size
                );

                break;
            }

            case MSG_WINDOW_DO_MOVE : {
                int error;
                msg_win_moved_t reply;

                error = send_ipc_message(
                    window->server_port,
                    MSG_WINDOW_DO_MOVE,
                    buffer,
                    sizeof( msg_win_do_move_t )
                );

                if ( error < 0 ) {
                    dbprintf( "window_thread(): Failed to send move request!\n" );
                    break;
                }

                error = recv_ipc_message(
                    window->reply_port,
                    NULL,
                    &reply,
                    sizeof( msg_win_moved_t ),
                    INFINITE_TIMEOUT
                );

                if ( error < 0 ) {
                    dbprintf( "window_thread(): Failed to get resize reply!\n" );
                }

                break;
            }

            case MSG_WINDOW_ACTIVATED :
                window_signal_event_handler( window, WE_ACTIVATED );
                break;

            case MSG_WINDOW_DEACTIVATED :
                window_signal_event_handler( window, WE_DEACTIVATED );
                break;

            case MSG_KEY_PRESSED : {
                msg_key_pressed_t* cmd;

                cmd = ( msg_key_pressed_t* )buffer;

                if ( window->focused_widget != NULL ) {
                    widget_key_pressed( window->focused_widget, cmd->key );
                }

                break;
            }

            case MSG_KEY_RELEASED : {
                msg_key_released_t* cmd;

                cmd = ( msg_key_released_t* )buffer;

                if ( window->focused_widget != NULL ) {
                    widget_key_released( window->focused_widget, cmd->key );
                }

                break;
            }

            case MSG_MOUSE_ENTERED : {
                point_t widget_position;
                msg_mouse_entered_t* cmd;

                cmd = ( msg_mouse_entered_t* )buffer;

                assert( window->mouse_widget == NULL );

                window->mouse_widget = window_find_widget_at( window, &cmd->mouse_position );

                assert( window->mouse_widget != NULL );

                /* Notify the widget */

                point_sub_n( &widget_position, &cmd->mouse_position, &window->mouse_widget->position );

                widget_mouse_entered( window->mouse_widget, &widget_position );

                break;
            }

            case MSG_MOUSE_EXITED :
                assert( window->mouse_widget != NULL );

                widget_mouse_exited( window->mouse_widget );
                window->mouse_widget = NULL;

                break;

            case MSG_MOUSE_MOVED : {
                msg_mouse_moved_t* cmd;
                point_t widget_position;
                widget_t* new_mouse_widget;

                cmd = ( msg_mouse_moved_t* )buffer;

                assert( window->mouse_widget != NULL );

                new_mouse_widget = window_find_widget_at( window, &cmd->mouse_position );

                assert( new_mouse_widget != NULL );

                point_sub_n( &widget_position, &cmd->mouse_position, &new_mouse_widget->position );

                if ( window->mouse_widget == new_mouse_widget ) {
                    widget_mouse_moved( window->mouse_widget, &widget_position );
                } else {
                    widget_mouse_exited( window->mouse_widget );
                    widget_mouse_entered( new_mouse_widget, &widget_position );

                    window->mouse_widget = new_mouse_widget;
                }

                break;
            }

            case MSG_MOUSE_PRESSED : {
                point_t widget_position;
                msg_mouse_pressed_t* cmd;

                cmd = ( msg_mouse_pressed_t* )buffer;

                assert( window->mouse_widget != NULL );
                assert( window->mouse_down_widget == NULL );

                /* Handle possible focus change */

                if ( window->focused_widget != window->mouse_widget ) {
                    if ( window->focused_widget != NULL ) {
                        /* TODO: this widget lost the focus */
                    }

                    window->focused_widget = window->mouse_widget;

                    /* TODO: the new widget is just got the focus */
                }

                /* Handle the mouse press event */

                window->mouse_down_widget = window->mouse_widget;

                point_sub_n( &widget_position, &cmd->mouse_position, &window->mouse_down_widget->position );

                widget_mouse_pressed( window->mouse_down_widget, &widget_position, cmd->button );

                break;
            }

            case MSG_MOUSE_RELEASED : {
                msg_mouse_released_t* cmd;

                cmd = ( msg_mouse_released_t* )buffer;

                assert( window->mouse_down_widget != NULL );

                widget_mouse_released( window->mouse_down_widget, cmd->button );

                window->mouse_down_widget = NULL;

                break;
            }

            case MSG_WINDOW_CALLBACK : {
                msg_window_callback_t* cmd;
                window_callback_t* callback;

                cmd = ( msg_window_callback_t* )buffer;
                callback = ( window_callback_t* )cmd->callback;

                callback( cmd->data );

                break;
            }

            case MSG_WINDOW_CLOSE_REQUEST :
                switch ( window->close_operation ) {
                    case WINDOW_HIDE :
                        send_ipc_message( window->server_port, MSG_WINDOW_HIDE, NULL, 0 );
                        break;

                    case WINDOW_DESTROY :
                        send_ipc_message( window->server_port, MSG_WINDOW_DESTROY, NULL, 0 );
                        running = 0;
                        break;
                }

                break;

            default :
                dbprintf( "window_thread(): Received unknown message: %x\n", code );
                break;
        }
    }

    window_destroy( window );
    free( buffer );

    return NULL;
}

int window_set_event_handler( window_t* window, int event, window_event_callback_t* callback, void* data ) {
    window_event_data_t* event_data;

    if ( ( window == NULL ) ||
         ( event < 0 ) ||
         ( event >= WE_COUNT ) ) {
        return -EINVAL;
    }

    event_data = &window->event_handlers[ event ];

    event_data->callback = callback;
    event_data->data = data;

    return 0;
}

int window_insert_callback( window_t* window, window_callback_t* callback, void* data ) {
    if ( gettid() == window->thread.thread_id ) {
        callback( data );
    } else {
        msg_window_callback_t msg;

        msg.callback = ( void* )callback;
        msg.data = data;

        send_ipc_message( window->client_port, MSG_WINDOW_CALLBACK, &msg, sizeof( msg_window_callback_t ) );
    }

    return 0;
}

window_t* create_window( const char* title, point_t* position, point_t* size, int flags ) {
    int error;
    window_t* window;
    size_t title_size;
    msg_create_win_t* request;
    msg_create_win_reply_t reply;

    /* Do some sanity checking */

    if ( title == NULL ) {
        goto error1;
    }

    title_size = strlen( title );

    /* Create the window object */

    window = window_create( size );

    if ( window == NULL ) {
        goto error1;
    }

    /* Register the window to the guiserver */

    request = ( msg_create_win_t* )malloc( sizeof( msg_create_win_t ) + title_size + 1 );

    if ( request == NULL ) {
        goto error2;
    }

    request->reply_port = window->reply_port;
    request->client_port = window->client_port;

    if ( position == NULL ) {
        point_init( &request->position, 0, 0 );
    } else {
        point_copy( &request->position, position );
    }

    if ( size == NULL ) {
        point_init( &request->size, 0, 0 );
    } else {
        point_copy( &request->size, size );
    }

    memcpy( ( void* )( request + 1 ), title, title_size + 1 );
    request->flags = flags;

    error = send_ipc_message( app_server_port, MSG_WINDOW_CREATE, request, sizeof( msg_create_win_t ) + title_size + 1 );

    free( request );

    if ( error < 0 ) {
        goto error2;
    }

    error = recv_ipc_message( window->reply_port, NULL, &reply, sizeof( msg_create_win_reply_t ), INFINITE_TIMEOUT );

    if ( ( error < 0 ) ||
         ( reply.server_port < 0 ) ) {
        goto error2;
    }

    window->close_operation = WINDOW_DESTROY;
    window->server_port = reply.server_port;

    error = pthread_create(
        &window->thread,
        NULL,
        window_thread,
        ( void* )window
    );

    if ( error != 0 ) {
        goto error3;
    }

    return window;

 error3:
    /* TODO: unregister the window from the guiserver */

 error2:
    window_destroy( window );

 error1:
    return NULL;
}

int window_resize( window_t* window, point_t* size ) {
    int error;
    msg_win_do_resize_t request;

    if ( ( window == NULL ) ||
         ( size == NULL ) ) {
        return -EINVAL;
    }

    request.reply_port = window->reply_port;
    point_copy( &request.size, size );

    error = send_ipc_message(
        window->client_port,
        MSG_WINDOW_DO_RESIZE,
        ( void* )&request,
        sizeof( msg_win_do_resize_t )
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int window_move( window_t* window, point_t* position ) {
    int error;
    msg_win_do_move_t request;

    if ( ( window == NULL ) ||
         ( position == NULL ) ) {
        return -EINVAL;
    }

    request.reply_port = window->reply_port;
    point_copy( &request.position, position );

    error = send_ipc_message(
        window->client_port,
        MSG_WINDOW_DO_MOVE,
        ( void* )&request,
        sizeof( msg_win_do_move_t )
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int window_show( window_t* window ) {
    int error;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = send_ipc_message( window->client_port, MSG_WINDOW_DO_SHOW, NULL, 0 );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int window_hide( window_t* window ) {
    int error;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = send_ipc_message( window->client_port, MSG_WINDOW_DO_HIDE, NULL, 0 );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
