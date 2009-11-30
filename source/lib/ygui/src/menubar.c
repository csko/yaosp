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

#include <ygui/menubar.h>

#include "internal.h"

static int menu_bar_paint( widget_t* widget, gc_t* gc ) {
    return 0;
}

static int menu_bar_get_preferred_size( widget_t* widget, point_t* size ) {
    int count;
    menu_bar_t* menubar;

    menubar = ( menu_bar_t* )widget_get_data( widget );
    count = array_get_size( &menubar->items );

    if ( count == 0 ) {
        point_init( size, -1, 0 );
    } else {
        int i;

        size->x = -1;
        size->y = 0;

        for ( i = 0; i < count; i++ ) {
            widget_t* item;
            point_t pref_size;

            item = ( widget_t* )array_get_item( &menubar->items, i );
            widget_get_preferred_size( item, &pref_size );

            size->y = MAX( size->y, pref_size.y );
        }
    }

    return 0;
}

static int menu_bar_validate( widget_t* widget ) {
    int i;
    int size;
    point_t position;
    menu_bar_t* menubar;

    menubar = ( menu_bar_t* )widget_get_data( widget );
    size = array_get_size( &menubar->items );

    point_init( &position, 0, 0 );

    for ( i = 0; i < size; i++ ) {
        widget_t* item;
        point_t pref_size;

        item = ( widget_t* )array_get_item( &menubar->items, i );
        widget_get_preferred_size( item, &pref_size );

        widget_set_position_and_size( item, &position, &pref_size );

        position.x += pref_size.x;
    }

    return 0;
}

static widget_operations_t menu_bar_ops = {
    .paint = menu_bar_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = menu_bar_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = menu_bar_validate,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL
};

widget_t* create_menubar( void ) {
    menu_bar_t* menu_bar;

    menu_bar = ( menu_bar_t* )malloc( sizeof( menu_bar_t ) );

    if ( menu_bar == NULL ) {
        goto error1;
    }

    if ( init_array( &menu_bar->items ) != 0 ) {
        goto error2;
    }

    menu_bar->active_item = NULL;

    return create_widget( W_MENUBAR, &menu_bar_ops, ( void* )menu_bar );

 error2:
    free( menu_bar );

 error1:
    return NULL;
}

int menubar_add_item( widget_t* bar, widget_t* item ) {
    menu_bar_t* menubar;
    menu_item_t* menuitem;

    if ( ( widget_get_id( bar ) != W_MENUBAR ) ||
         ( widget_get_id( item ) != W_MENUITEM ) ) {
        return -EINVAL;
    }

    menubar = ( menu_bar_t* )widget_get_data( bar );
    menuitem = ( menu_item_t* )widget_get_data( item );

    array_add_item( &menubar->items, ( void* )item );
    widget_inc_ref( item );

    widget_add( bar, item, NULL );

    menuitem->parent.bar = menubar;
    menuitem->parent_type = M_PARENT_BAR;

    return 0;
}
