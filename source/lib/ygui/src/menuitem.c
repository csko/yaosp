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
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <ygui/menuitem.h>
#include <ygui/menu.h>

#include "internal.h"

enum {
    E_CLICKED,
    E_COUNT
};

static int menu_item_events[ E_COUNT ] = {
    -1
};

static event_type_t menu_item_event_types[ E_COUNT ] = {
    { "clicked", &menu_item_events[ E_CLICKED ] }
};

static color_t fg_color = { 0, 0, 0, 255 };
static color_t bg_color = { 216, 216, 216, 255 };
static color_t active_bg_color = { 101, 152, 202, 255 };

static int menu_item_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    point_t position;
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Fill the background of the menuitem */

    if ( item->active ) {
        gc_set_pen_color( gc, &active_bg_color );
    } else {
        gc_set_pen_color( gc, &bg_color );
    }

    gc_fill_rect( gc, &bounds );

    /* Draw the image */

    position.x = 3;

    if ( item->image != NULL ) {
        position.y = ( rect_height( &bounds ) - bitmap_get_height( item->image ) ) / 2;

        gc_set_drawing_mode( gc, DM_BLEND );
        gc_draw_bitmap( gc, &position, item->image );
        gc_set_drawing_mode( gc, DM_COPY );

        position.x += bitmap_get_width( item->image );
        position.x += 3;
    }

    /* Draw the text to the menuitem */

    int asc = font_get_ascender( item->font );
    int desc = font_get_descender( item->font );

    position.y = ( rect_height( &bounds ) - ( asc - desc ) ) / 2 + asc;

    gc_translate_xy( gc, 1, 1 );
    rect_resize( &bounds, 0, 0, -2, -2 );

    gc_set_pen_color( gc, &fg_color );
    gc_set_font( gc, item->font );
    gc_set_clip_rect( gc, &bounds );

    gc_draw_text( gc, &position, item->text, -1 );

    return 0;
}

static int menu_item_show_submenu( widget_t* widget, menu_item_t* item ) {
    point_t tmp;
    point_t position;

    if ( item->submenu == NULL ) {
        return 0;
    }

    window_get_position( widget->window, &position );
    widget_get_position( widget, &tmp );

    point_add( &position, &tmp );
    point_add_xy( &position, 1, 21 ); /* todo: this is a quick hack to add the decorator's size */

    widget_get_preferred_size( widget, &tmp );
    tmp.x = 0;
    point_add( &position, &tmp );

    menu_popup_at( item->submenu, &position );

    return 0;
}

static int menu_item_mouse_entered( widget_t* widget, point_t* position ) {
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_data( widget );
    item->active = 1;

    switch ( item->parent_type ) {
        case M_PARENT_BAR : {
            menu_bar_t* bar = item->parent.bar;

            if ( bar->active_item != NULL ) {
                if ( bar->active_item->submenu != NULL ) {
                    menu_close( bar->active_item->submenu );
                }

                menu_item_show_submenu( widget, item );

                bar->active_item = item;
            }

            break;
        }

        case M_PARENT_MENU :
            break;

        case M_PARENT_NONE :
            break;
    }

    widget_invalidate( widget );

    return 0;
}

static int menu_item_mouse_exited( widget_t* widget ) {
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_data( widget );
    item->active = 0;

    widget_invalidate( widget );

    return 0;
}

static int menu_item_mouse_released( widget_t* widget, int mount_button ) {
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_data( widget );

    /* Hide the window menu */

    switch ( item->parent_type ) {
        case M_PARENT_MENU :
            menu_close( item->parent.menu );
            break;

        case M_PARENT_BAR : {
            menu_bar_t* bar = item->parent.bar;

            if ( bar->active_item == NULL ) {
                menu_item_show_submenu( widget, item );
                bar->active_item = item;
            }

            break;
        }

        case M_PARENT_NONE :
            assert( 0 );
            break;
    }

    /* Fire event listeners */

    widget_signal_event_handler( widget, menu_item_events[ E_CLICKED ] );

    return 0;
}

static int menu_item_get_preferred_size( widget_t* widget, point_t* size ) {
    int img_width;
    int img_height;
    int text_height;
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_data( widget );

    if ( item->image == NULL ) {
        img_width = 0;
        img_height = 0;
    } else {
        img_width = bitmap_get_width( item->image ) + 3 /* for spacing */;
        img_height = bitmap_get_height( item->image );
    }

    text_height = font_get_height( item->font );

    point_init(
        size,
        img_width + font_get_string_width( item->font, item->text, -1 ) + 6,
        MAX( img_height, text_height ) + 6
    );

    return 0;
}

static widget_operations_t menu_item_ops = {
    .paint = menu_item_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = menu_item_mouse_entered,
    .mouse_exited = menu_item_mouse_exited,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = menu_item_mouse_released,
    .get_minimum_size = NULL,
    .get_preferred_size = menu_item_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL
};

widget_t* create_menuitem_with_label( const char* text ) {
    return create_menuitem_with_label_and_image( text, NULL );
}

widget_t* create_menuitem_with_label_and_image( const char* text, bitmap_t* image ) {
    widget_t* widget;
    menu_item_t* item;
    font_properties_t properties;

    item = ( menu_item_t* )malloc( sizeof( menu_item_t ) );

    if ( item == NULL ) {
        goto error1;
    }

    item->text = strdup( text );

    if ( item->text == NULL ) {
        goto error2;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    item->font = create_font( "DejaVu Sans", "Book", &properties );

    if ( item->font == NULL ) {
        goto error3;
    }

    widget = create_widget( W_MENUITEM, &menu_item_ops, item );

    if ( widget == NULL ) {
        goto error4;
    }

    if ( widget_add_events( widget, menu_item_event_types, menu_item_events, E_COUNT ) != 0 ) {
        goto error5;
    }

    item->active = 0;
    item->image = image;
    item->submenu = NULL;
    item->parent.menu = NULL;
    item->parent_type = M_PARENT_NONE;

    if ( image != NULL ) {
        bitmap_inc_ref( image );
    }

    return widget;

 error5:
    /* TODO: free the widget */

 error4:
    destroy_font( item->font );

 error3:
    free( item->text );

 error2:
    free( item );

 error1:
    return NULL;
}

static int separator_menu_item_paint( widget_t* widget, gc_t* gc ) {
    rect_t tmp;
    rect_t bounds;

    widget_get_bounds( widget, &bounds );

    /* Fill the background of the menuitem */

    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    /* Draw the separator line */

    rect_init(
        &tmp,
        3,
        rect_height( &bounds ) / 2,
        bounds.right - 3,
        rect_height( &bounds ) / 2
    );

    gc_set_pen_color( gc, &fg_color );
    gc_fill_rect( gc, &tmp );

    return 0;
}

static int separator_menu_item_get_preferred_size( widget_t* widget, point_t* size ) {
    point_init( size, 0, 7 );

    return 0;
}

static widget_operations_t separator_menu_item_ops = {
    .paint = separator_menu_item_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = separator_menu_item_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL
};

widget_t* create_separator_menuitem( void ) {
    return create_widget( W_SEPARATOR_MENUITEM, &separator_menu_item_ops, NULL );
}

int menuitem_has_image( widget_t* widget ) {
    menu_item_t* item;

    if ( widget_get_id( widget ) != W_MENUITEM ) {
        return 0;
    }

    item = ( menu_item_t* )widget_get_data( widget );

    return ( item->image != NULL );
}

int menuitem_set_submenu( widget_t* widget, menu_t* menu ) {
    menu_item_t* item;

    if ( widget_get_id( widget ) != W_MENUITEM ) {
        return -EINVAL;
    }

    item = ( menu_item_t* )widget_get_data( widget );
    item->submenu = menu;

    menu->parent = widget;

    return 0;
}

int menuitem_menu_closed( widget_t* widget ) {
    menu_item_t* item;

    if ( widget_get_id( widget ) != W_MENUITEM ) {
        return -EINVAL;
    }

    item = ( menu_item_t* )widget_get_data( widget );

    switch ( item->parent_type ) {
        case M_PARENT_BAR :
            item->parent.bar->active_item = NULL;
            break;

        case M_PARENT_MENU :
            break;

        case M_PARENT_NONE :
            break;
    }

    return 0;
}
