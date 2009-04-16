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

#include <ygui/protocol.h>

#include "../internal.h"

typedef struct r_buf_header {
    ipc_port_id reply_port;
} __attribute__(( packed )) r_buf_header_t;

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
