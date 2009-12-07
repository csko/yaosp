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
#include <sys/param.h>

#include <ygui/menu.h>
#include <ygui/border/lineborder.h>

#include "internal.h"

static int menu_window_deactivated( window_t* window, void* data ) {
    menu_t* menu;

    menu = ( menu_t* )data;
    menu_close( menu );

    return 0;
}

menu_t* create_menu( void ) {
    menu_t* menu;
    border_t* border;
    widget_t* container;

    menu = ( menu_t* )malloc( sizeof( menu_t ) );

    if ( menu == NULL ) {
        goto error1;
    }

    if ( init_array( &menu->items ) != 0 ) {
        goto error2;
    }

    array_set_realloc_size( &menu->items, 16 );

    menu->window = create_window( "menu", NULL, NULL, WINDOW_NO_BORDER );

    if ( menu->window == NULL ) {
        goto error3;
    }

    container = window_get_container( menu->window );

    border = create_line_border();
    widget_set_border( container, border );
    border_dec_ref( border );

    window_set_event_handler( menu->window, WE_DEACTIVATED, menu_window_deactivated, menu );

    menu->parent = NULL;

    return menu;

 error3:
    destroy_array( &menu->items );

 error2:
    free( menu );

 error1:
    return NULL;
}

int menu_add_item( menu_t* menu, widget_t* item ) {
    int item_id;
    widget_t* container;

    item_id = widget_get_id( item );

    switch ( item_id ) {
        case W_MENUITEM : {
            menu_item_t* menu_item = ( menu_item_t* )widget_get_data( item );

            if ( menu_item->parent.menu != NULL ) {
                return -EINVAL;
            }

            menu_item->parent.menu = menu;
            menu_item->parent_type = M_PARENT_MENU;

            break;
        }

        case W_SEPARATOR_MENUITEM :
            break;

        default :
            return -EINVAL;
    }

    array_add_item( &menu->items, ( void* )item );

    container = window_get_container( menu->window );
    widget_add( container, item, NULL );

    return 0;
}

int menu_get_size( menu_t* menu, point_t* size ) {
    int i;
    int count;

    point_init( size, 0, 0 );

    count = array_get_size( &menu->items );

    for ( i = 0; i < count; i++ ) {
        widget_t* item;
        point_t  preferred_size;

        item = ( widget_t* )array_get_item( &menu->items, i );

        widget_get_preferred_size( item, &preferred_size );

        size->x = MAX( size->x, preferred_size.x );
        size->y += preferred_size.y;
    }

    /* line border ... */

    point_add_xy( size, 6, 6 );

    return 0;
}

int menu_popup_at( menu_t* menu, point_t* position ) {
    int i;
    int size;
    point_t item_pos;
    point_t win_size;

    if ( ( menu == NULL ) ||
         ( position == NULL ) ) {
        return -EINVAL;
    }

    point_init( &win_size, 0, 0 );
    point_init( &item_pos, 0, 0 );

    size = array_get_size( &menu->items );

    for ( i = 0; i < size; i++ ) {
        widget_t* item;
        point_t pref_size;

        item = ( widget_t* )array_get_item( &menu->items, i );

        widget_get_preferred_size( item, &pref_size );

        win_size.y += pref_size.y;
        win_size.x = MAX( win_size.x, pref_size.x );
    }

    for ( i = 0; i < size; i++ ) {
        widget_t* item;
        point_t pref_size;

        item = ( widget_t* )array_get_item( &menu->items, i );

        widget_get_preferred_size( item, &pref_size );
        pref_size.x = win_size.x;

        widget_set_position_and_size(
            item,
            &item_pos,
            &pref_size
        );

        item_pos.y += pref_size.y;
    }

    point_add_xy( &win_size, 6, 6 ); /* line border */
    window_resize( menu->window, &win_size );

    window_move( menu->window, position );

    window_show( menu->window );

    return 0;
}

int menu_popup_at_xy( menu_t* menu, int x, int y ) {
    point_t tmp = {
        .x = x,
        .y = y
    };

    return menu_popup_at( menu, &tmp );
}

int menu_close( menu_t* menu ) {
    if ( menu->parent != NULL ) {
        menuitem_menu_closed( menu->parent );
    }

    window_hide( menu->window );

    return 0;
}
