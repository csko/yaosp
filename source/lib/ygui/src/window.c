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

static widget_t* window_find_widget_at( window_t* window, point_t* _position ) {
    widget_t* widget;
    point_t position;

    widget = window->container;

    point_copy( &position, _position );

    while ( 1 ) {
        int i;
        int size;
        int found;

        found = 0;
        size = array_get_size( &widget->children );

        for ( i = 0; i < size; i++ ) {
            widget_wrapper_t* wrapper;

            wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, i );

            rect_t widget_rect = {
                .left = 0,
                .top = 0,
                .right = widget->visible_size.x - 1,
                .bottom = widget->visible_size.y - 1
            };

            rect_add_point( &widget_rect, &wrapper->widget->position );
            rect_add_point( &widget_rect, &wrapper->widget->scroll_offset );

            if ( rect_has_point( &widget_rect, &position ) ) {
                point_sub( &position, &widget->position );
                point_sub( &position, &widget->scroll_offset );

                widget = wrapper->widget;
                found = 1;

                break;
            }
        }

        if ( !found ) {
            return widget;
        }
    }

    /* Muhaha :) */

    return NULL;
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

                flush_render_buffer( window );

                /* Show the window (if needed) */

                if ( code == MSG_DO_SHOW_WINDOW ) {
                    send_ipc_message( window->server_port, MSG_SHOW_WINDOW, NULL, 0 );
                }

                break;
            }

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
                //assert( ( cmd->mouse_position.x >= 0 ) && ( cmd->mouse_position.x < window->container->size.x ) );
                //assert( ( cmd->mouse_position.y >= 0 ) && ( cmd->mouse_position.y < window->container->size.y ) );

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
                //assert( ( cmd->mouse_position.x >= 0 ) && ( cmd->mouse_position.x < window->container->size.x ) );
                //assert( ( cmd->mouse_position.y >= 0 ) && ( cmd->mouse_position.y < window->container->size.y ) );

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

    memset( window, 0, sizeof( window_t ) );

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

    point_t container_position = { .x = 0, .y = 0 };

    widget_set_position_and_size(
        window->container,
        &container_position,
        size
    );

    window->render_buffer = ( uint8_t* )malloc( DEFAULT_RENDER_BUFFER_SIZE );

    if ( window->render_buffer == NULL ) {
        goto error5;
    }

    initialize_render_buffer( window );
    window->render_buffer_max_size = DEFAULT_RENDER_BUFFER_SIZE;

    /* Initialize the graphics context of the window */

    error = init_gc( window, &window->gc );

    if ( error < 0 ) {
        goto error6;
    }

    /* Register the window to the guiserver */

    request = ( msg_create_win_t* )malloc( sizeof( msg_create_win_t ) + title_size + 1 );

    if ( request == NULL ) {
        goto error6;
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
        goto error6;
    }

    error = recv_ipc_message( window->reply_port, NULL, &reply, sizeof( msg_create_win_reply_t ), INFINITE_TIMEOUT );

    if ( error < 0 ) {
        goto error6;
    }

    if ( reply.server_port < 0 ) {
        goto error6;
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
        goto error7;
    }

    wake_up_thread( thread );

    return window;

 error7:
    /* TODO: Unregister the window from the guiserver */

 error6:
    /* TODO: free the render buffer */

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
