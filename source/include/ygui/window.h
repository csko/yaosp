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

#ifndef _YGUI_WINDOW_H_
#define _YGUI_WINDOW_H_

#include <sys/types.h>

#include <yaosp/ipc.h>

#include <ygui/point.h>
#include <ygui/widget.h>
#include <ygui/gc.h>

typedef struct window {
    ipc_port_id client_port;
    ipc_port_id server_port;
    ipc_port_id reply_port;

    widget_t* container;
    widget_t* focused_widget;
    widget_t* mouse_widget;
    widget_t* mouse_down_widget;

    gc_t gc;
    uint8_t* render_buffer;
    size_t render_buffer_size;
    size_t render_buffer_max_size;
} window_t;

enum {
    WINDOW_NO_BORDER = ( 1 << 0 )
};

widget_t* window_get_container( window_t* window );

window_t* create_window( const char* title, point_t* position, point_t* size, int flags );

int show_window( window_t* window );
int hide_window( window_t* window );

#endif /* _YGUI_WINDOW_H_ */
