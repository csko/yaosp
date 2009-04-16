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

#ifndef _YAOSP_WIDGET_H_
#define _YAOSP_WIDGET_H_

#include <yutil/array.h>

#include <ygui/point.h>
#include <ygui/rect.h>
#include <ygui/color.h>

enum {
    W_PANEL = 1
};

struct window;
struct widget;

typedef struct widget_operations {
    int ( *paint )( struct widget* widget );
} widget_operations_t;

typedef struct widget {
    int id;
    void* data;
    struct window* window;
    widget_operations_t* ops;

    point_t position;
    point_t size;

    array_t children;
} widget_t;

int widget_add( widget_t* parent, widget_t* child );

int widget_get_id( widget_t* widget );
void* widget_get_data( widget_t* widget );
int widget_get_bounds( widget_t* widget, rect_t* bounds );

int widget_set_window( widget_t* widget, struct window* window );

int widget_paint( widget_t* widget );

int widget_set_pen_color( widget_t* widget, color_t* color );
int widget_fill_rect( widget_t* widget, rect_t* rect );

widget_t* create_widget( int id, widget_operations_t* ops, void* data );

#endif /* _YAOSP_WIDGET_H_ */
