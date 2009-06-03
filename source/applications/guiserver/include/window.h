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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <ygui/protocol.h>
#include <ygui/color.h>

#include <region.h>
#include <bitmap.h>
#include <fontmanager.h>

typedef struct window {
    char* title;
    int flags;
    rect_t screen_rect;
    rect_t client_rect;

    ipc_port_id client_port;
    ipc_port_id server_port;

    bitmap_t* bitmap;
    region_t visible_regions;

    int is_visible;
    int is_moving;
    int mouse_on_decorator;
    void* decorator_data;

    /* Rendering stuffs */

    color_t pen_color;
    font_node_t* font;
} window_t;

int handle_create_window( msg_create_win_t* request );

int window_mouse_entered( window_t* window, point_t* mouse_position );
int window_mouse_exited( window_t* window );
int window_mouse_moved( window_t* window, point_t* mouse_position );
int window_mouse_pressed( window_t* window, int button );
int window_mouse_released( window_t* window, int button );

#endif /* _WINDOW_H_ */
