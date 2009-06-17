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

    /* Repaint the widget if it has a valid
       paint method and the widget is invalid */

    if ( ( !widget->is_valid ) &&
         ( widget->ops->paint != NULL ) ) {
        /* Set the current clip rect to the widget rect */

        rect_t widget_rect = {
            .left = 0,
            .top = 0,
            .right = widget->size.x - 1,
            .bottom = widget->size.y - 1
        };

        widget_set_clip_rect( widget, &widget_rect );

        /* Call the paint method of the widget */

        widget->ops->paint( widget );

        /* The widget is valid now :) */

        widget->is_valid = 1;
    }

    /* Call paint on the children */

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

int widget_key_pressed( widget_t* widget, int key ) {
    if ( widget->ops->key_pressed == NULL ) {
        return 0;
    }

    return widget->ops->key_pressed( widget, key );
}

int widget_key_released( widget_t* widget, int key ) {
    if ( widget->ops->key_released == NULL ) {
        return 0;
    }

    return widget->ops->key_released( widget, key );
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

int widget_mouse_pressed( widget_t* widget, point_t* position, int mouse_button ) {
    if ( widget->ops->mouse_pressed == NULL ) {
        return 0;
    }

    return widget->ops->mouse_pressed( widget, position, mouse_button );
}

int widget_mouse_released( widget_t* widget, int mouse_button ) {
    if ( widget->ops->mouse_released == NULL ) {
        return 0;
    }

    return widget->ops->mouse_released( widget, mouse_button );
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

int widget_set_clip_rect( widget_t* widget, rect_t* rect ) {
    int error;
    window_t* window;
    r_set_clip_rect_t* packet;

    window = widget->window;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = allocate_render_packet( window, sizeof( r_set_clip_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_SET_CLIP_RECT;

    rect_add_point_n( &packet->clip_rect, rect, &widget->position );

    rect_t widget_rect = {
        .left = widget->position.x,
        .top = widget->position.y,
        .right = widget->position.x + widget->size.x - 1,
        .bottom = widget->position.y + widget->size.y - 1
    };

    rect_and( &packet->clip_rect, &widget_rect );

    return 0;
}

int widget_draw_rect( widget_t* widget, rect_t* rect ) {
    int error;
    window_t* window;
    r_draw_rect_t* packet;

    window = widget->window;

    if ( window == NULL ) {
        return -EINVAL;
    }

    error = allocate_render_packet( window, sizeof( r_draw_rect_t ), ( void** )&packet );

    if ( error < 0 ) {
        return error;
    }

    packet->header.command = R_DRAW_RECT;

    rect_add_point_n( &packet->rect, rect, &widget->position );

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

    rect_add_point_n( &packet->rect, rect, &widget->position );

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

    point_add_n( &packet->position, position, &widget->position );
    memcpy( ( void* )( packet + 1 ), text, length );

    return 0;
}

static int widget_find_event_handler( widget_t* widget, const char* name, int* pos ) {
    int first;
    int last;
    int mid;
    int result;
    event_entry_t* tmp;

    first = 0;
    last = array_get_size( &widget->event_handlers ) - 1;

    while ( first <= last ) {
        mid = ( first + last ) / 2;
        tmp = ( event_entry_t* )array_get_item( &widget->event_handlers, mid );

        result = strcmp( name, tmp->name );

        if ( result < 0 ) {
            last = mid - 1;
        } else if ( result > 0 ) {
            first = mid + 1;
        } else {
            if ( pos != NULL ) {
                *pos = mid;
            }

            return 0;
        }
    }

    if ( pos != NULL ) {
        *pos = first;
    }

    return -ENOENT;
}

int widget_connect_event_handler( widget_t* widget, const char* event_name, event_callback_t* callback, void* data ) {
    int pos;
    int error;
    event_entry_t* entry;

    error = widget_find_event_handler( widget, event_name, &pos );

    if ( error < 0 ) {
        return error;
    }

    entry = ( event_entry_t* )array_get_item( &widget->event_handlers, pos );

    entry->callback = callback;
    entry->data = data;

    return 0;
}

int widget_signal_event_handler( widget_t* widget, int event_handler ) {
    event_entry_t* entry;

    if ( ( event_handler < 0 ) ||
         ( event_handler >= array_get_size( &widget->event_handlers ) ) ) {
        return -EINVAL;
    }

    entry = ( event_entry_t* )array_get_item( &widget->event_handlers, event_handler );

    if ( entry->callback != NULL ) {
        entry->callback( widget, entry->data );
    }

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

    error = init_array( &widget->event_handlers );

    if ( error < 0 ) {
        goto error3;
    }

    array_set_realloc_size( &widget->children, 8 );

    widget->id = id;
    widget->data = data;
    widget->ref_count = 1;

    widget->ops = ops;
    widget->window = NULL;
    widget->is_valid = 0;

    return widget;

error3:
    destroy_array( &widget->children );

error2:
    free( widget );

error1:
    return NULL;
}

int widget_set_events( widget_t* widget, event_type_t* event_types, int* event_indexes, int event_count ) {
    int i;
    int j;
    int pos;
    int error;
    int* index;
    event_type_t* type;
    event_entry_t* entry;

    /* Insert the event types */

    for ( i = 0, type = event_types; i < event_count; i++, type++ ) {
        entry = ( event_entry_t* )malloc( sizeof( event_entry_t ) );

        if ( entry == NULL ) {
            return -ENOMEM;
        }

        entry->name = type->name;
        entry->callback = NULL;

        error = widget_find_event_handler( widget, type->name, &pos );

        if ( error == 0 ) {
            return -EEXIST;
        }

        error = array_insert_item( &widget->event_handlers, pos, ( void* )entry );

        if ( error < 0 ) {
            return error;
        }
    }

    /* Calculate event handler indexes */

    for ( i = 0, type = event_types, index = event_indexes; i < event_count; i++, type++, index++ ) {
        for ( j = 0; j < event_count; j++ ) {
            entry = ( event_entry_t* )array_get_item( &widget->event_handlers, j );

            if ( strcmp( type->name, entry->name ) == 0 ) {
                *index = j;

                break;
            }
        }

        assert( j != event_count );
    }

    return 0;
}
