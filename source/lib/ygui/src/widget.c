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
#include <assert.h>

#include <ygui/widget.h>
#include <ygui/protocol.h>
#include <ygui/render/render.h>

#include "internal.h"

int widget_add( widget_t* parent, widget_t* child ) {
    widget_inc_ref( child );

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

widget_t* widget_get_child_at( widget_t* widget, point_t* position ) {
    int i;
    int size;
    widget_t* tmp;
    rect_t widget_rect;

    size = array_get_size( &widget->children );

    if ( size == 0 ) {
        return widget;
    }

    for ( i = 0; i < size; i++ ) {
        tmp = ( widget_t* )array_get_item( &widget->children, i );

        rect_init(
            &widget_rect,
            tmp->position.x,
            tmp->position.y,
            tmp->position.x + tmp->size.x - 1,
            tmp->position.y + tmp->size.y - 1
        );

        if ( rect_has_point( &widget_rect, position ) ) {
            return tmp;
        }
    }

    return widget;
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

int widget_set_position( widget_t* widget, point_t* position ) {
    memcpy( &widget->position, position, sizeof( point_t ) );

    return 0;
}

int widget_set_size( widget_t* widget, point_t* size ) {
    memcpy( &widget->size, size, sizeof( point_t ) );

    return 0;
}

int widget_inc_ref( widget_t* widget ) {
    assert( widget->ref_count > 0 );

    widget->ref_count++;

    return 0;
}

int widget_dec_ref( widget_t* widget ) {
    assert( widget->ref_count > 0 );

    if ( --widget->ref_count == 0 ) {
        /* TODO: free the widget! */
    }

    return 0;
}

int widget_paint( widget_t* widget ) {
    int i;
    int size;
    widget_t* child;

    if ( ( !widget->is_valid ) &&
         ( widget->ops->paint != NULL ) ) {
        widget->ops->paint( widget );

        widget->is_valid = 1;
    }

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        child = ( widget_t* )array_get_item( &widget->children, i );

        widget_paint( child );
    }

    return 0;
}

int widget_invalidate( widget_t* widget, int notify_window ) {
    int i;
    int size;
    widget_t* tmp;

    widget->is_valid = 0;

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        tmp = ( widget_t* )array_get_item( &widget->children, i );

        widget_invalidate( tmp, 0 );
    }

    if ( ( notify_window ) &&
         ( widget->window != NULL ) ) {
        send_ipc_message( widget->window->client_port, MSG_WIDGET_INVALIDATED, NULL, 0 );
    }

    return 0;
}

int widget_mouse_entered( widget_t* widget, point_t* position ) {
    if ( widget->ops->mouse_entered == NULL ) {
        return 0;
    }

    return widget->ops->mouse_entered( widget, position );
}

int widget_mouse_exited( widget_t* widget ) {
    if ( widget->ops->mouse_exited == NULL ) {
        return 0;
    }

    return widget->ops->mouse_exited( widget );
}

int widget_mouse_moved( widget_t* widget, point_t* position ) {
    if ( widget->ops->mouse_moved == NULL ) {
        return 0;
    }

    return widget->ops->mouse_moved( widget, position );
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

int widget_set_font( widget_t* widget, font_t* font ) {
    int error;
    window_t* window;
    r_set_font_t* packet;

    window = widget->window;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = allocate_render_packet( window, sizeof( r_set_font_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_FONT;
    packet->font_handle = font->handle;

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

    rect_add_point( &packet->rect, &widget->position );

    return 0;
}

int widget_draw_text( widget_t* widget, point_t* position, const char* text, int length ) {
    int error;
    window_t* window;
    r_draw_text_t* packet;

    if ( text == NULL ) {
        return -EINVAL;
    }

    if ( length == -1 ) {
        length = strlen( text );
    }

    if ( length == 0 ) {
        return 0;
    }

    window = widget->window;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = allocate_render_packet( window, sizeof( r_draw_text_t ) + length, ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_DRAW_TEXT;
    packet->length = length;

    memcpy( &packet->position, position, sizeof( point_t ) );
    memcpy( ( void* )( packet + 1 ), text, length );

    point_add( &packet->position, &widget->position );

    return 0;
}

widget_t* create_widget( int id, widget_operations_t* ops, void* data ) {
    int error;
    widget_t* widget;

    widget = ( widget_t* )malloc( sizeof( widget_t ) );

    if ( widget == NULL ) {
        goto error1;
    }

    memset( widget, 0, sizeof( widget_t ) );

    error = init_array( &widget->children );

    if ( error < 0 ) {
        goto error2;
    }

    array_set_realloc_size( &widget->children, 8 );

    widget->id = id;
    widget->data = data;
    widget->ref_count = 1;

    widget->ops = ops;
    widget->window = NULL;
    widget->is_valid = 0;

    return widget;

error2:
    free( widget );

error1:
    return NULL;
}
