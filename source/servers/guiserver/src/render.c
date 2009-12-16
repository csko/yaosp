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

#include <yaosp/debug.h>

#include <ygui/render/render.h>

#include <window.h>
#include <windowdecorator.h>
#include <graphicsdriver.h>
#include <windowmanager.h>

typedef struct r_buf_header {
    ipc_port_id reply_port;
} __attribute__(( packed )) r_buf_header_t;

extern window_decorator_t* window_decorator;

static int set_pen_color( window_t* window, uint8_t* buffer ) {
    r_set_pen_color_t* cmd;

    cmd = ( r_set_pen_color_t* )buffer;

    memcpy( &window->pen_color, &cmd->color, sizeof( color_t ) );

    return sizeof( r_set_pen_color_t );
}

static int set_font( window_t* window, uint8_t* buffer ) {
    r_set_font_t* cmd;

    cmd = ( r_set_font_t* )buffer;

    window->font = ( font_node_t* )cmd->font_handle;

    return sizeof( r_set_font_t );
}

static int set_clip_rect( window_t* window, uint8_t* buffer ) {
    r_set_clip_rect_t* cmd;

    cmd = ( r_set_clip_rect_t* )buffer;

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        rect_add_point_n(
            &window->clip_rect,
            &cmd->clip_rect,
            &window_decorator->lefttop_offset
        );
    } else {
        rect_copy( &window->clip_rect, &cmd->clip_rect );
    }

    return sizeof( r_set_clip_rect_t );
}

static int set_drawing_mode( window_t* window, uint8_t* buffer ) {
    r_set_drawing_mode_t* cmd;

    cmd = ( r_set_drawing_mode_t* )buffer;

    window->drawing_mode = cmd->mode;

    return sizeof( r_set_drawing_mode_t );
}

static int draw_rect( window_t* window, uint8_t* buffer ) {
    rect_t tmp;
    rect_t rect;
    r_draw_rect_t* cmd;

    cmd = ( r_draw_rect_t* )buffer;

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        rect_add_point_n(
            &rect,
            &cmd->rect,
            &window_decorator->lefttop_offset
        );
    } else {
        rect_copy( &rect, &cmd->rect );
    }

    /* Top line */

    rect_init( &tmp, rect.left, rect.top, rect.right, rect.top );
    rect_and( &tmp, &window->clip_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect(
            window->bitmap,
            &tmp,
            &window->pen_color,
            window->drawing_mode
        );
    }

    /* Bottom line */

    rect_init( &tmp, rect.left, rect.bottom, rect.right, rect.bottom );
    rect_and( &tmp, &window->clip_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect(
            window->bitmap,
            &tmp,
            &window->pen_color,
            window->drawing_mode
        );
    }

    /* Left line */

    rect_init( &tmp, rect.left, rect.top, rect.left, rect.bottom );
    rect_and( &tmp, &window->clip_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect(
            window->bitmap,
            &tmp,
            &window->pen_color,
            window->drawing_mode
        );
    }

    /* Right line */

    rect_init( &tmp, rect.right, rect.top, rect.right, rect.bottom );
    rect_and( &tmp, &window->clip_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect(
            window->bitmap,
            &tmp,
            &window->pen_color,
            window->drawing_mode
        );
    }

    return sizeof( r_draw_rect_t );
}

static int fill_rect( window_t* window, uint8_t* buffer ) {
    rect_t tmp;
    r_fill_rect_t* cmd;

    cmd = ( r_fill_rect_t* )buffer;

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        rect_add_point_n(
            &tmp,
            &cmd->rect,
            &window_decorator->lefttop_offset
        );
    } else {
        rect_copy( &tmp, &cmd->rect );
    }

    rect_and( &tmp, &window->clip_rect );

    if ( rect_is_valid( &tmp ) ) {
        graphics_driver->fill_rect(
            window->bitmap,
            &tmp,
            &window->pen_color,
            window->drawing_mode
        );
    }

    return sizeof( r_fill_rect_t );
}

static int draw_text( window_t* window, uint8_t* buffer ) {
    r_draw_text_t* cmd;

    cmd = ( r_draw_text_t* )buffer;

    if ( window->font == NULL ) {
        goto out;
    }

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        point_add(
            &cmd->position,
            &window_decorator->lefttop_offset
        );
    }

    graphics_driver->draw_text(
        window->bitmap,
        &cmd->position,
        &window->clip_rect,
        window->font,
        &window->pen_color,
        ( const char* )( cmd + 1 ),
        cmd->length
    );

 out:
    return ( sizeof( r_draw_text_t ) + cmd->length );
}

static int draw_bitmap( window_t* window, uint8_t* buffer ) {
    rect_t tmp;
    point_t lefttop;
    bitmap_t* bitmap;
    r_draw_bitmap_t* cmd;

    cmd = ( r_draw_bitmap_t* )buffer;

    bitmap = bitmap_get( cmd->bitmap_id );

    if ( bitmap == NULL ) {
        goto out;
    }

    if ( ( window->flags & WINDOW_NO_BORDER ) == 0 ) {
        point_add(
            &cmd->position,
            &window_decorator->lefttop_offset
        );
    }

    rect_init(
        &tmp,
        0,
        0,
        bitmap->width - 1,
        bitmap->height - 1
    );
    rect_add_point( &tmp, &cmd->position );
    rect_and( &tmp, &window->clip_rect );

    if ( !rect_is_valid( &tmp ) ) {
        goto out;
    }

    rect_lefttop( &tmp, &lefttop );
    rect_sub_point( &tmp, &cmd->position );

    graphics_driver->blit_bitmap(
        window->bitmap,
        &lefttop,
        bitmap,
        &tmp,
        window->drawing_mode
    );

    bitmap_put( bitmap );

 out:
    return sizeof( r_draw_bitmap_t );
}

static int render_done( window_t* window ) {
    if ( window->is_visible ) {
        pthread_mutex_lock( &wm_lock );
        wm_update_window_region( window, &window->client_rect );
        pthread_mutex_unlock( &wm_lock );
    }

    return sizeof( render_header_t );
}

int window_do_render( window_t* window, uint8_t* buffer, int size ) {
    uint8_t* buffer_end;
    r_buf_header_t* header;

    header = ( r_buf_header_t* )buffer;

    if ( window->bitmap == NULL ) {
        dbprintf( "window_do_render(): Tried to render to a window without bitmap! Skipping ...\n" );
        goto out;
    }

    buffer_end = buffer + size;

    size -= sizeof( r_buf_header_t );
    buffer += sizeof( r_buf_header_t );

    while ( buffer != buffer_end ) {
        render_header_t* r_header;

        r_header = ( render_header_t* )buffer;

        switch ( r_header->command ) {
            case R_SET_PEN_COLOR :
                buffer += set_pen_color( window, buffer );
                break;

            case R_SET_FONT :
                buffer += set_font( window, buffer );
                break;

            case R_SET_CLIP_RECT :
                buffer += set_clip_rect( window, buffer );
                break;

            case R_SET_DRAWING_MODE :
                buffer += set_drawing_mode( window, buffer );
                break;

            case R_DRAW_RECT :
                buffer += draw_rect( window, buffer );
                break;

            case R_FILL_RECT :
                buffer += fill_rect( window, buffer );
                break;

            case R_DRAW_TEXT :
                buffer += draw_text( window, buffer );
                break;

            case R_DRAW_BITMAP :
                buffer += draw_bitmap( window, buffer );
                break;

            case R_DONE :
                buffer += render_done( window );
                break;

            default :
                dbprintf( "window_do_render(): Invalid render command: %x\n", r_header->command );
                break;
        }
    }

 out:
    /* Tell the window that rendering is done */

    send_ipc_message( header->reply_port, 0, NULL, 0 );

    return 0;
}

