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
#include <ygui/render/render.h>

#include <window.h>
#include <windowdecorator.h>
#include <windowmanager.h>
#include <graphicsdriver.h>
#include <mouse.h>

#define MAX_WINDOW_BUFSIZE 8192

typedef struct r_buf_header {
    ipc_port_id reply_port;
} __attribute__(( packed )) r_buf_header_t;

extern window_decorator_t* window_decorator;

static int window_do_render( window_t* window, uint8_t* buffer, int size ) {
    r_buf_header_t* header;

    header = ( r_buf_header_t* )buffer;

    size -= sizeof( r_buf_header_t );
    buffer += sizeof( r_buf_header_t );

    while ( size > 0 ) {
        render_header_t* r_header;

        r_header = ( render_header_t* )buffer;

        switch ( r_header->command ) {
            case R_SET_PEN_COLOR : {
                r_set_pen_color_t* cmd;

                cmd = ( r_set_pen_color_t* )buffer;

                memcpy( &window->pen_color, &cmd->color, sizeof( color_t ) );

                buffer += sizeof( r_set_pen_color_t );
                size -= sizeof( r_set_pen_color_t );

                break;
            }

            case R_SET_FONT : {
                r_set_font_t* cmd;

                cmd = ( r_set_font_t* )buffer;

                window->font = ( font_node_t* )cmd->font_handle;

                buffer += sizeof( r_set_font_t );
                size -= sizeof( r_set_font_t );

                break;
            }

            case R_DRAW_RECT : {
                rect_t tmp;
                rect_t rect;
                r_draw_rect_t* cmd;

                cmd = ( r_draw_rect_t* )buffer;

                rect_add_point_n( &rect, &cmd->rect, &window_decorator->lefttop_offset );

                rect_init( &tmp, rect.left, rect.top, rect.right, rect.top );

                graphics_driver->fill_rect(
                    window->bitmap,
                    &tmp,
                    &window->pen_color,
                    DM_COPY
                );

                rect_init( &tmp, rect.left, rect.bottom, rect.right, rect.bottom );

                graphics_driver->fill_rect(
                    window->bitmap,
                    &tmp,
                    &window->pen_color,
                    DM_COPY
                );

                rect_init( &tmp, rect.left, rect.top, rect.left, rect.bottom );

                graphics_driver->fill_rect(
                    window->bitmap,
                    &tmp,
                    &window->pen_color,
                    DM_COPY
                );

                rect_init( &tmp, rect.right, rect.top, rect.right, rect.bottom );

                graphics_driver->fill_rect(
                    window->bitmap,
                    &tmp,
                    &window->pen_color,
                    DM_COPY
                );

                break;
            }

            case R_FILL_RECT : {
                r_fill_rect_t* cmd;

                cmd = ( r_fill_rect_t* )buffer;

                rect_add_point( &cmd->rect, &window_decorator->lefttop_offset );

                graphics_driver->fill_rect(
                    window->bitmap,
                    &cmd->rect,
                    &window->pen_color,
                    DM_COPY
                );

                buffer += sizeof( r_fill_rect_t );
                size -= sizeof( r_fill_rect_t );

                break;
            }

            case R_DRAW_TEXT : {
                r_draw_text_t* cmd;

                cmd = ( r_draw_text_t* )buffer;

                if ( window->font == NULL ) {
                    goto r_draw_text_done;
                }

                point_add( &cmd->position, &window_decorator->lefttop_offset );

                graphics_driver->draw_text(
                    window->bitmap,
                    &cmd->position,
                    &screen_rect,
                    window->font,
                    &window->pen_color,
                    ( const char* )( cmd + 1 ),
                    cmd->length
                );

r_draw_text_done:
                buffer += sizeof( r_draw_text_t ) + cmd->length;
                size -= ( sizeof( r_draw_text_t ) + cmd->length );

                break;
            }

            case R_DONE :
                if ( window->is_visible ) {
                    LOCK( wm_lock );

                    wm_update_window_region( window, &window->client_rect );

                    UNLOCK( wm_lock );
                }

                buffer += sizeof( render_header_t );
                size -= sizeof( render_header_t );

                break;

            default :
                dbprintf( "window_do_render(): Invalid render command: %x\n", r_header->command );
                break;
        }
    }

    /* Tell the window that rendering is done */

    send_ipc_message( header->reply_port, 0, NULL, 0 );

    return 0;
}

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
            case MSG_RENDER_COMMANDS :
                window_do_render( window, ( uint8_t* )buffer, error );
                break;

            case MSG_SHOW_WINDOW :
                wm_register_window( window );
                window->is_visible = 1;
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
    int error;
    int width;
    int height;
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

    error = init_region( &window->visible_regions );

    if ( error < 0 ) {
        goto error4;
    }

    window->client_port = request->client_port;
    window->flags = request->flags;
    window->is_visible = 0;
    window->is_moving = 0;
    window->mouse_on_decorator = 0;

    /* Initialize rendering stuffs */

    window->font = NULL;

    rect_init(
        &window->screen_rect,
        request->position.x,
        request->position.y,
        request->position.x + request->size.x + window_decorator->border_size.x - 1,
        request->position.y + request->size.y + window_decorator->border_size.y - 1
    );
    rect_init(
        &window->client_rect,
        request->position.x + window_decorator->lefttop_offset.x,
        request->position.y + window_decorator->lefttop_offset.y,
        request->position.x + window_decorator->lefttop_offset.x + request->size.x - 1,
        request->position.y + window_decorator->lefttop_offset.y + request->size.y - 1
    );

    rect_bounds( &window->screen_rect, &width, &height );

    window->bitmap = create_bitmap( width, height, CS_RGB32 );

    if ( window->bitmap == NULL ) {
        goto error5;
    }

    error = window_decorator->initialize( window );

    if ( error < 0 ) {
        goto error6;
    }

    window_decorator->calculate_regions( window );

    thread = create_thread(
        "window",
        PRIORITY_DISPLAY,
        window_thread,
        window,
        0
    );

    if ( thread < 0 ) {
        goto error7;
    }

    wake_up_thread( thread );

    reply.server_port = window->server_port;

    goto out;

error7:
    window_decorator->destroy( window );

error6:
    put_bitmap( window->bitmap );

error5:
    destroy_region( &window->visible_regions );

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

int window_mouse_entered( window_t* window, point_t* mouse_position ) {
    point_t window_position;

    point_sub_xy_n( &window_position, mouse_position, window->screen_rect.left, window->screen_rect.top );

    window->mouse_on_decorator = window_decorator->border_has_position( window, mouse_position );

    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_entered( window, mouse_position );
    } else {
        msg_mouse_entered_t cmd;

        memcpy( &cmd.mouse_position, mouse_position, sizeof( point_t ) );
        point_sub_xy( &cmd.mouse_position, window->client_rect.left, window->client_rect.top );

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

    on_decorator = window_decorator->border_has_position( window, mouse_position );

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

            memcpy( &cmd.mouse_position, mouse_position, sizeof( point_t ) );
            point_sub_xy( &cmd.mouse_position, window->client_rect.left, window->client_rect.top );

            send_ipc_message( window->client_port, MSG_MOUSE_ENTERED, &cmd, sizeof( msg_mouse_entered_t ) );
        } else {
            msg_mouse_moved_t cmd;

            memcpy( &cmd.mouse_position, mouse_position, sizeof( point_t ) );
            point_sub_xy( &cmd.mouse_position, window->client_rect.left, window->client_rect.top );

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
        cmd.button = mouse_button;

        send_ipc_message( window->client_port, MSG_MOUSE_PRESSED, &cmd, sizeof( msg_mouse_pressed_t ) );
    }

    return 0;
}

int window_mouse_released( window_t* window, int mouse_button ) {
    if ( window->mouse_on_decorator ) {
        window_decorator->mouse_released( window, mouse_button );
    } else {
        msg_mouse_released_t cmd;

        cmd.button = mouse_button;

        send_ipc_message( window->client_port, MSG_MOUSE_RELEASED, &cmd, sizeof( msg_mouse_released_t ) );
    }

    return 0;
}
