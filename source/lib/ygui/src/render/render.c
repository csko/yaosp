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

#include <errno.h>
#include <yaosp/debug.h>

#include <ygui/protocol.h>
#include <ygui/render/render.h>

#include "../internal.h"

typedef struct r_buf_header {
    ipc_port_id reply_port;
} __attribute__(( packed )) r_buf_header_t;

#if 0
static int dump_render_buffer( window_t* window ) {
    int remaining;
    uint8_t* data;
    r_buf_header_t* buf_header;

    buf_header = ( r_buf_header_t* )window->render_buffer;

    dbprintf( "Dumping render buffer: size = %d, reply_port = %d\n", window->render_buffer_size, buf_header->reply_port );

    data = window->render_buffer + sizeof( r_buf_header_t );
    remaining = window->render_buffer_size - sizeof( r_buf_header_t );

    while ( remaining > 0 ) {
        render_header_t* header;

        header = ( render_header_t* )data;

        switch ( header->command ) {
            case R_SET_PEN_COLOR : {
                r_set_pen_color_t* cmd = ( r_set_pen_color_t* )header;
                dbprintf( "  set pen color {%d,%d,%d}\n", cmd->color.red, cmd->color.green, cmd->color.blue );
                remaining -= sizeof( r_set_pen_color_t );
                data += sizeof( r_set_pen_color_t );
                break;
            }

            case R_SET_FONT : {
                r_set_font_t* cmd = ( r_set_font_t* )header;
                dbprintf( "  set font %d\n", cmd->font_handle );
                remaining -= sizeof( r_set_font_t );
                data += sizeof( r_set_font_t );
                break;
            }

            case R_SET_CLIP_RECT : {
                r_set_clip_rect_t* cmd = ( r_set_clip_rect_t* )header;
                dbprintf( "  set clip rect {%d,%d,%d,%d}\n", cmd->clip_rect.left, cmd->clip_rect.top, cmd->clip_rect.right, cmd->clip_rect.bottom );
                remaining -= sizeof( r_set_clip_rect_t );
                data += sizeof( r_set_clip_rect_t );
                break;
            }

            case R_SET_DRAWING_MODE : {
                r_set_drawing_mode_t* cmd = ( r_set_drawing_mode_t* )header;
                dbprintf( "  set drawing mode %d\n", cmd->mode );
                remaining -= sizeof( r_set_drawing_mode_t );
                data += sizeof( r_set_drawing_mode_t );
                break;
            }

            case R_DRAW_RECT : {
                r_draw_rect_t* cmd = ( r_draw_rect_t* )header;
                dbprintf( "  draw rect {%d,%d,%d,%d}\n", cmd->rect.left, cmd->rect.top, cmd->rect.right, cmd->rect.bottom );
                remaining -= sizeof( r_draw_rect_t );
                data += sizeof( r_draw_rect_t );
                break;
            }

            case R_FILL_RECT : {
                r_fill_rect_t* cmd = ( r_fill_rect_t* )header;
                dbprintf( "  fill rect {%d,%d,%d,%d}\n", cmd->rect.left, cmd->rect.top, cmd->rect.right, cmd->rect.bottom );
                remaining -= sizeof( r_fill_rect_t );
                data += sizeof( r_fill_rect_t );
                break;
            }

            case R_DRAW_TEXT : {
                r_draw_text_t* cmd = ( r_draw_text_t* )header;
                char buf[ cmd->length + 1 ];
                memcpy( buf, cmd + 1, cmd->length );
                buf[ cmd->length ] = 0;
                dbprintf( "  draw text '%s'\n", buf );
                remaining -= sizeof( r_draw_text_t );
                remaining -= cmd->length;
                data += sizeof( r_draw_text_t );
                data += cmd->length;
                break;
            }

            case R_DRAW_BITMAP : {
                r_draw_bitmap_t* cmd = ( r_draw_bitmap_t* )header;
                dbprintf( "  draw bitmap %d\n", cmd->bitmap_id );
                remaining -= sizeof( r_draw_bitmap_t );
                data += sizeof( r_draw_bitmap_t );
                break;
            }

            case R_DONE :
                remaining -= sizeof( render_header_t );
                data += sizeof( render_header_t );
                dbprintf( "  done\n" );
                break;

            default :
                dbprintf( "  unknown!\n" );
                return 0;
        }
    }

    return 0;
}
#endif

int initialize_render_buffer( window_t* window ) {
    r_buf_header_t* header;

    header = ( r_buf_header_t* )window->render_buffer;
    header->reply_port = window->reply_port;

    window->render_buffer_size = sizeof( r_buf_header_t );

    return 0;
}

int allocate_render_packet( window_t* window, size_t size, void** buffer ) {
    int error;

    if ( size > window->render_buffer_max_size ) {
        return -E2BIG;
    }

    if ( ( window->render_buffer_size + size ) > window->render_buffer_max_size ) {
        error = flush_render_buffer( window );

        if ( error < 0 ) {
            return error;
        }
    }

    *buffer = ( void* )( window->render_buffer + window->render_buffer_size );

    window->render_buffer_size += size;

    return 0;
}

int flush_render_buffer( window_t* window ) {
    int error;

    if ( window->render_buffer_size == 0 ) {
        return 0;
    }

    //dump_render_buffer( window );

    error = send_ipc_message( window->server_port, MSG_RENDER_COMMANDS, window->render_buffer, window->render_buffer_size );

    if ( error < 0 ) {
        return error;
    }

    error = recv_ipc_message( window->reply_port, NULL, NULL, 0, INFINITE_TIMEOUT );

    if ( error < 0 ) {
        return error;
    }

    error = initialize_render_buffer( window );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
