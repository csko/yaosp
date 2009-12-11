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

#ifndef _YGUI_WIDGET_H_
#define _YGUI_WIDGET_H_

#include <yutil/array.h>

#include <ygui/point.h>
#include <ygui/rect.h>
#include <ygui/color.h>
#include <ygui/font.h>
#include <ygui/event.h>
#include <ygui/gc.h>
#include <ygui/border/border.h>

/* Global widget types */

enum {
    W_PANEL = 1,
    W_LABEL,
    W_BUTTON,
    W_TEXTFIELD,
    W_TEXTAREA,
    W_SCROLLPANEL,
    W_IMAGE,
    W_MENUITEM,
    W_SEPARATOR_MENUITEM,
    W_MENUBAR,
    W_DIRVIEW,
    W_TYPE_COUNT
};

/* Global widget events */

enum {
    E_PREF_SIZE_CHANGED,
    E_VIEWPORT_CHANGED,
    E_MOUSE_DOWN,
    E_WIDGET_COUNT
};

struct window;
struct widget;

typedef struct widget_operations {
    int ( *paint )( struct widget* widget, gc_t* gc );
    int ( *key_pressed )( struct widget* widget, int key );
    int ( *key_released )( struct widget* widget, int key );
    int ( *mouse_entered )( struct widget* widget, point_t* position );
    int ( *mouse_exited )( struct widget* widget );
    int ( *mouse_moved )( struct widget* widget, point_t* position );
    int ( *mouse_pressed )( struct widget* widget, point_t* position, int mouse_button );
    int ( *mouse_released )( struct widget* widget, int mouse_button );
    int ( *get_minimum_size )( struct widget* widget, point_t* size );
    int ( *get_preferred_size )( struct widget* widget, point_t* size );
    int ( *get_maximum_size )( struct widget* widget, point_t* size );
    int ( *get_viewport )( struct widget* widget, rect_t* viewport );
    int ( *do_validate )( struct widget* widget );
    int ( *size_changed )( struct widget* widget );
    int ( *added_to_window )( struct widget* widget );
    int ( *child_added )( struct widget* widget, struct widget* child );
    int ( *destroy )( struct widget* widget );
} widget_operations_t;

typedef struct widget {
    int id;
    void* data;
    int ref_count;

    struct window* window;
    widget_operations_t* ops;

    int is_valid;

    point_t position;
    point_t scroll_offset;
    point_t full_size;
    point_t visible_size;

    int is_pref_size_set;
    point_t preferred_size;

    array_t children;
    border_t* border;
    struct widget* parent;

    /* Event handling */

    int event_ids[ E_WIDGET_COUNT ];
    array_t event_handlers;
} widget_t;

int widget_add( widget_t* parent, widget_t* child, void* data );

int widget_get_id( widget_t* widget );
void* widget_get_data( widget_t* widget );
int widget_get_child_count( widget_t* widget );
widget_t* widget_get_child_at( widget_t* widget, int index );
int widget_get_bounds( widget_t* widget, rect_t* bounds );
int widget_get_minimum_size( widget_t* widget, point_t* size );
int widget_get_preferred_size( widget_t* widget, point_t* size );
int widget_get_maximum_size( widget_t* widget, point_t* size );
int widget_get_viewport( widget_t* widget, rect_t* viewport );
int widget_get_position( widget_t* widget, point_t* position );
int widget_get_scroll_offset( widget_t* widget, point_t* offset );
int widget_get_size( widget_t* widget, point_t* size );

int widget_set_window( widget_t* widget, struct window* window );
int widget_set_position_and_size( widget_t* widget, point_t* position, point_t* size );
int widget_set_position_and_sizes( widget_t* widget, point_t* position, point_t* visible_size, point_t* full_size );
int widget_set_preferred_size( widget_t* widget, point_t* size );
int widget_set_scroll_offset( widget_t* widget, point_t* scroll_offset );
int widget_set_border( widget_t* widget, border_t* border );

int widget_inc_ref( widget_t* widget );
int widget_dec_ref( widget_t* widget );
int widget_paint( widget_t* widget, gc_t* gc );
int widget_invalidate( widget_t* widget );

int widget_key_pressed( widget_t* widget, int key );
int widget_key_released( widget_t* widget, int key );
int widget_mouse_entered( widget_t* widget, point_t* position );
int widget_mouse_exited( widget_t* widget );
int widget_mouse_moved( widget_t* widget, point_t* position );
int widget_mouse_pressed( widget_t* widget, point_t* position, int mouse_button );
int widget_mouse_released( widget_t* widget, int mouse_button );

/* Event related functions */

int widget_connect_event_handler( widget_t* widget, const char* event_name, event_callback_t* callback, void* data );
int widget_signal_event_handler( widget_t* widget, int event_handler );

widget_t* create_widget( int id, widget_operations_t* ops, void* data );

int widget_add_events( widget_t* widget, event_type_t* event_types, int* event_indexes, int event_count );

#endif /* _YGUI_WIDGET_H_ */
