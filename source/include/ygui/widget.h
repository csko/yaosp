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
#include <ygui/font.h>
#include <ygui/event.h>

enum {
    W_PANEL = 1,
    W_LABEL,
    W_BUTTON
};

struct window;
struct widget;

typedef struct widget_operations {
    int ( *paint )( struct widget* widget );
    int ( *mouse_entered )( struct widget* widget, point_t* position );
    int ( *mouse_exited )( struct widget* widget );
    int ( *mouse_moved )( struct widget* widget, point_t* position );
    int ( *mouse_pressed )( struct widget* widget, point_t* position, int mouse_button );
    int ( *mouse_released )( struct widget* widget, int mouse_button );
} widget_operations_t;

typedef struct widget {
    int id;
    void* data;
    int ref_count;

    struct window* window;
    widget_operations_t* ops;

    int is_valid;

    point_t position;
    point_t size;

    array_t children;

    /* Event handling */

    array_t event_handlers;
} widget_t;

int widget_add( widget_t* parent, widget_t* child );

int widget_get_id( widget_t* widget );
void* widget_get_data( widget_t* widget );
int widget_get_bounds( widget_t* widget, rect_t* bounds );
widget_t* widget_get_child_at( widget_t* widget, point_t* position );

int widget_set_window( widget_t* widget, struct window* window );
int widget_set_position( widget_t* widget, point_t* position );
int widget_set_size( widget_t* widget, point_t* size );

int widget_inc_ref( widget_t* widget );
int widget_dec_ref( widget_t* widget );
int widget_paint( widget_t* widget );
int widget_invalidate( widget_t* widget, int notify_window );

int widget_mouse_entered( widget_t* widget, point_t* position );
int widget_mouse_exited( widget_t* widget );
int widget_mouse_moved( widget_t* widget, point_t* position );
int widget_mouse_pressed( widget_t* widget, point_t* position, int mouse_button );
int widget_mouse_released( widget_t* widget, int mouse_button );

/* Drawing functions */

int widget_set_pen_color( widget_t* widget, color_t* color );
int widget_set_font( widget_t* widget, font_t* font );
int widget_fill_rect( widget_t* widget, rect_t* rect );
int widget_draw_text( widget_t* widget, point_t* position, const char* text, int length );

/* Event related functions */

int widget_connect_event_handler( widget_t* widget, const char* event_name, event_callback_t* callback, void* data );
int widget_signal_event_handler( widget_t* widget, int event_handler );

widget_t* create_widget( int id, widget_operations_t* ops, void* data );

int widget_set_events( widget_t* widget, event_type_t* event_types, int* event_indexes, int event_count );

#endif /* _YAOSP_WIDGET_H_ */
