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

#include "internal.h"

static int do_invalidate_widget( widget_t* widget, int notify_window ) {
    int i;
    int size;

    widget->is_valid = 0;

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        widget_wrapper_t* wrapper;

        wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, i );
        do_invalidate_widget( wrapper->widget, 0 );
    }

    if ( ( notify_window ) &&
         ( widget->window != NULL ) ) {
        send_ipc_message( widget->window->client_port, MSG_WIDGET_INVALIDATED, NULL, 0 );
    }

    return 0;
}

int widget_add( widget_t* parent, widget_t* child, void* data ) {
    widget_wrapper_t* wrapper;

    wrapper = ( widget_wrapper_t* )malloc( sizeof( widget_wrapper_t ) );

    if ( wrapper == NULL ) {
        return -ENOMEM;
    }

    widget_inc_ref( child );

    wrapper->widget = child;
    wrapper->data = data;

    array_add_item( &parent->children, ( void* )wrapper );

    if ( parent->window != NULL ) {
        widget_set_window( child, parent->window );
    }

    child->parent = parent;

    /* Notify the parent widget about the new child */

    if ( parent->ops->child_added != NULL ) {
        parent->ops->child_added( parent, child );
    }

    return 0;
}

int widget_get_id( widget_t* widget ) {
    return widget->id;
}

void* widget_get_data( widget_t* widget ) {
    return widget->data;
}

int widget_get_child_count( widget_t* widget ) {
    return array_get_size( &widget->children );
}

widget_t* widget_get_child_at( widget_t* widget, int index ) {
    widget_wrapper_t* wrapper;

    if ( ( index < 0 ) ||
         ( index >= array_get_size( &widget->children ) ) ) {
        return NULL;
    }

    wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, index );

    return wrapper->widget;
}

int widget_get_bounds( widget_t* widget, rect_t* bounds ) {
    rect_init(
        bounds,
        0, 0,
        widget->full_size.x - 1, widget->full_size.y - 1
    );

    if ( widget->border != NULL ) {
        bounds->right -= widget->border->size.x;
        bounds->bottom -= widget->border->size.y;
    }

    return 0;
}

int widget_get_minimum_size( widget_t* widget, point_t* size ) {
    if ( widget->ops->get_minimum_size == NULL ) {
        point_init( size, 0, 0 );
    } else {
        widget->ops->get_minimum_size( widget, size );
    }

    return 0;
}

int widget_get_preferred_size( widget_t* widget, point_t* size ) {
    if ( widget->is_pref_size_set ) {
        point_copy( size, &widget->preferred_size );
    } else if ( widget->ops->get_preferred_size != NULL ) {
        widget->ops->get_preferred_size( widget, size );
    } else {
        point_init( size, 0, 0 );
    }

    /* The border size is included in the preferred size of the widget */

    if ( widget->border != NULL ) {
        point_add( size, &widget->border->size );
    }

    return 0;
}

int widget_get_maximum_size( widget_t* widget, point_t* size ) {
    if ( widget->ops->get_maximum_size == NULL ) {
        point_init( size, INT_MAX, INT_MAX );
    } else {
        widget->ops->get_maximum_size( widget, size );
    }

    return 0;
}

int widget_get_viewport( widget_t* widget, rect_t* viewport ) {
    if ( widget->ops->get_viewport == NULL ) {
        rect_init( viewport, 0, 0, 0, 0 );
    } else {
        widget->ops->get_viewport( widget, viewport );
    }

    return 0;
}

int widget_get_position( widget_t* widget, point_t* position ) {
    point_copy( position, &widget->position );

    return 0;
}

int widget_get_scroll_offset( widget_t* widget, point_t* offset ) {
    point_copy( offset, &widget->scroll_offset );

    return 0;
}

int widget_get_size( widget_t* widget, point_t* size ) {
    point_copy( size, &widget->full_size );

    return 0;
}

int widget_set_window( widget_t* widget, struct window* window ) {
    int i;
    int size;

    widget->window = window;

    if ( widget->ops->added_to_window != NULL ) {
        widget->ops->added_to_window( widget );
    }

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        widget_wrapper_t* wrapper;

        wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, i );

        widget_set_window( wrapper->widget, window );
    }

    return 0;
}

int widget_set_position_and_size( widget_t* widget, point_t* position, point_t* size ) {
    return widget_set_position_and_sizes( widget, position, size, size );
}

int widget_set_position_and_sizes( widget_t* widget, point_t* position, point_t* visible_size, point_t* full_size ) {
    /* Update the position and the size of the widget */

    point_copy( &widget->position, position );
    point_copy( &widget->visible_size, visible_size );
    point_copy( &widget->full_size, full_size );

    /* Tell the widget that its size is changed */

    if ( widget->ops->size_changed != NULL ) {
        widget->ops->size_changed( widget );
    }

    /* As the size of the widget is changed, we should repaint it ... */

    do_invalidate_widget( widget, 0 );

    return 0;
}

int widget_set_preferred_size( widget_t* widget, point_t* size ) {
    point_copy( &widget->preferred_size, size );
    widget->is_pref_size_set = 1;

    return 0;
}

int widget_set_scroll_offset( widget_t* widget, point_t* scroll_offset ) {
    if ( scroll_offset != NULL ) {
        point_copy( &widget->scroll_offset, scroll_offset );
    } else {
        point_init( &widget->scroll_offset, 0, 0 );
    }

    return 0;
}

int widget_set_border( widget_t* widget, border_t* border ) {
    if ( ( widget == NULL ) ||
         ( border == NULL ) ) {
        return -EINVAL;
    }

    if ( widget->border != NULL ) {
        border_dec_ref( widget->border );
    }

    widget->border = border;
    border_inc_ref( widget->border );

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
        if ( widget->ops->destroy != NULL ) {
            widget->ops->destroy( widget );
        }

        free( widget );
    }

    return 0;
}

int widget_paint( widget_t* widget, gc_t* gc ) {
    int i;
    int size;
    rect_t res_area;

    /* Calculate the restricted area of the current widget */

    rect_init(
        &res_area,
        0,
        0,
        widget->visible_size.x - 1,
        widget->visible_size.y - 1
    );
    rect_add_point( &res_area, &gc->lefttop );
    rect_and( &res_area, gc_current_restricted_area( gc ) );

    /* If the restricted area is not valid, then we can make sure that
       none of the child widgets will be visible, so painting can be
       terminated here! */

    if ( !rect_is_valid( &res_area ) ) {
        return 0;
    }

    /* Push the restricted area */

    gc_push_restricted_area( gc, &res_area );

    /* Paint the border and transfer the left-top
       position according to the border */

    if ( widget->border != NULL ) {
        gc_push_translate_checkpoint( gc );
        gc_translate( gc, &widget->scroll_offset );

        widget->border->ops->paint( widget, gc );

        gc_rollback_translate( gc );

        point_add( &gc->lefttop, &widget->border->lefttop_offset );
    }

    /* Repaint the widget if it has a valid
       paint method and the widget is invalid */

    if ( !widget->is_valid ) {
        /* Validate the widget */

        if ( widget->ops->do_validate != NULL ) {
            widget->ops->do_validate( widget );
        }

        /* Paint the widget */

        if ( widget->ops->paint != NULL ) {
            gc_push_translate_checkpoint( gc );
            gc_translate( gc, &widget->scroll_offset );

            widget->ops->paint( widget, gc );

            gc_rollback_translate( gc );
        }

        /* The widget is valid now :) */

        widget->is_valid = 1;
    }

    /* Call paint on the children */

    size = array_get_size( &widget->children );

    for ( i = 0; i < size; i++ ) {
        widget_t* child;
        widget_wrapper_t* wrapper;

        wrapper = array_get_item( &widget->children, i );
        child = wrapper->widget;

        gc_push_translate_checkpoint( gc );
        gc_translate( gc, &child->position );

        widget_paint( child, gc );

        gc_rollback_translate( gc );
    }

    /* Bring the left-top position back to the original */

    if ( widget->border != NULL ) {
        point_sub( &gc->lefttop, &widget->border->lefttop_offset );
    }

    /* Pop the restricted area */

    gc_pop_restricted_area( gc );

    return 0;
}

int widget_invalidate( widget_t* widget ) {
    do_invalidate_widget( widget, 1 );
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
    widget_signal_event_handler( widget, widget->event_ids[ E_MOUSE_DOWN ] );

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
    widget_t* widget;

    widget = ( widget_t* )calloc( 1, sizeof( widget_t ) );

    if ( widget == NULL ) {
        goto error1;
    }

    if ( init_array( &widget->children ) != 0 ) {
        goto error2;
    }

    if ( init_array( &widget->event_handlers ) != 0 ) {
        goto error3;
    }

    array_set_realloc_size( &widget->children, 8 );

    /* Add widget related events */

    event_type_t widget_events[] = {
        { "preferred-size-changed", &widget->event_ids[ E_PREF_SIZE_CHANGED ] },
        { "viewport-changed", &widget->event_ids[ E_VIEWPORT_CHANGED ] },
        { "mouse-down", &widget->event_ids[ E_MOUSE_DOWN ] }
    };

    if ( widget_add_events( widget, widget_events, widget->event_ids, E_WIDGET_COUNT ) != 0 ) {
        goto error4;
    }

    widget->id = id;
    widget->data = data;
    widget->ref_count = 1;
    widget->border = NULL;
    widget->parent = NULL;

    widget->ops = ops;
    widget->window = NULL;
    widget->is_valid = 0;

    return widget;

 error4:
    destroy_array( &widget->event_handlers );

 error3:
    destroy_array( &widget->children );

 error2:
    free( widget );

 error1:
    return NULL;
}

int widget_add_events( widget_t* widget, event_type_t* event_types, int* event_indexes, int event_count ) {
    int i;
    int pos;
    int size;
    int error;
    event_type_t* type;
    event_entry_t* entry;

    /* Insert the event types */

    for ( i = 0, type = event_types; i < event_count; i++, type++ ) {
        entry = ( event_entry_t* )malloc( sizeof( event_entry_t ) );

        if ( entry == NULL ) {
            return -ENOMEM;
        }

        entry->name = type->name;
        entry->event_id = type->event_id;
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

    size = array_get_size( &widget->event_handlers );

    for ( i = 0; i < size; i++ ) {
        entry = ( event_entry_t* )array_get_item( &widget->event_handlers, i );

        *entry->event_id = i;
    }

    return 0;
}
