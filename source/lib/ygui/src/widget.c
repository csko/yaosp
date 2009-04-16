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

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <ygui/widget.h>
#include <ygui/render/render.h>

#include "internal.h"

int widget_add( widget_t* parent, widget_t* child ) {
    array_add_item( &parent->children, ( void* )child );

    if ( parent->window != NULL ) {
        widget_set_window( child, parent->window );
    }

    return 0;
}

int widget_get_id( widget_t* widget ) {
    return widget->id;
}

void* widget_get_data( widget_t* widget ) {
    return widget->data;
}

int widget_get_bounds( widget_t* widget, rect_t* bounds ) {
    rect_init(
        bounds,
        0,
        0,
        widget->size.x - 1,
        widget->size.y - 1
    );

    return 0;
}

int widget_set_window( widget_t* widget, struct window* window ) {
    int i;
    int size;
    widget_t* child;

    widget->window = window;

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        child = ( widget_t* )array_get_item( &widget->children, i );

        widget_set_window( child, window );
    }

    return 0;
}

int widget_paint( widget_t* widget ) {
    if ( widget->ops->paint != NULL ) {
        widget->ops->paint( widget );
    }

    return 0;
}

int widget_set_pen_color( widget_t* widget, color_t* color ) {
    int error;
    window_t* window;
    r_set_pen_color_t* packet;

    window = widget->window;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = allocate_render_packet( window, sizeof( r_set_pen_color_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_PEN_COLOR;
    memcpy( &packet->color, color, sizeof( color_t ) );

    return 0;
}

int widget_fill_rect( widget_t* widget, rect_t* rect ) {
    int error;
    window_t* window;
    r_fill_rect_t* packet;

    window = widget->window;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = allocate_render_packet( window, sizeof( r_fill_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_FILL_RECT;
    memcpy( &packet->rect, rect, sizeof( rect_t ) );

    return 0;
}

widget_t* create_widget( int id, widget_operations_t* ops, void* data ) {
    int error;
    widget_t* widget;

    widget = ( widget_t* )malloc( sizeof( widget_t ) );

    if ( widget == NULL ) {
        goto error1;
    }

    error = init_array( &widget->children );

    if ( error < 0 ) {
        goto error2;
    }

    array_set_realloc_size( &widget->children, 8 );

    widget->id = id;
    widget->ops = ops;
    widget->data = data;
    widget->window = NULL;

    return widget;

error2:
    free( widget );

error1:
    return NULL;
}
