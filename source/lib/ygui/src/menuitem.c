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

#include <ygui/menuitem.h>

typedef struct menu_item {
    char* text;
    font_t* font;
    bitmap_t* image;
    int active;
} menu_item_t;

static int menu_item_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    point_t position;
    menu_item_t* item;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 216, 216, 216, 0xFF };
    static color_t active_bg_color = { 116, 116, 166, 0xFF };

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

    position.x = 2;

    if ( item->image != NULL ) {
        position.y = ( rect_height( &bounds ) - bitmap_get_height( item->image ) ) / 2;

        gc_set_drawing_mode( gc, DM_BLEND );
        gc_draw_bitmap( gc, item->image, &position );
        gc_set_drawing_mode( gc, DM_COPY );

        position.x += bitmap_get_width( item->image );
        position.x += 2;
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

static int menu_item_mouse_entered( widget_t* widget, point_t* position ) {
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_data( widget );
    item->active = 1;

    widget_invalidate( widget, 1 );

    return 0;
}

static int menu_item_mouse_exited( widget_t* widget ) {
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_data( widget );
    item->active = 0;

    widget_invalidate( widget, 1 );

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
        img_width = bitmap_get_width( item->image ) + 2 /* for spacing */;
        img_height = bitmap_get_height( item->image );
    }

    text_height = font_get_height( item->font );

    point_init(
        size,
        img_width + font_get_string_width( item->font, item->text, -1 ) + 4,
        MAX( img_height, text_height ) + 4
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
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = menu_item_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL
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

    item->active = 0;
    item->image = image;

    if ( image != NULL ) {
        bitmap_inc_ref( image );
    }

    return widget;

error4:
    /* TODO: free the font */

error3:
    free( item->text );

error2:
    free( item );

error1:
    return NULL;
}

int menuitem_has_image( widget_t* widget ) {
    menu_item_t* item;

    if ( widget_get_id( widget ) != W_MENUITEM ) {
        return 0;
    }

    item = ( menu_item_t* )widget_get_data( widget );

    return ( item->image != NULL );
}
