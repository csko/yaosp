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

#ifndef _WINDOWDECORATOR_H_
#define _WINDOWDECORATOR_H_

#include <ygui/point.h>

#include <window.h>

typedef struct window_decorator {
    point_t border_size;
    point_t lefttop_offset;
    int ( *initialize )( window_t* window );
    int ( *destroy )( window_t* window );
    int ( *calculate_regions )( window_t* window );
    int ( *update_border )( window_t* window );
    int ( *border_has_position )( window_t* window, point_t* position );
    int ( *mouse_entered )( window_t* window, point_t* position );
    int ( *mouse_exited )( window_t* window );
    int ( *mouse_moved )( window_t* window, point_t* position );
    int ( *mouse_pressed )( window_t* window, int button );
    int ( *mouse_released )( window_t* window, int button );
} window_decorator_t;

extern window_decorator_t default_decorator;
extern window_decorator_t* window_decorator;

#endif /* _WINDOWDECORATOR_H_ */
