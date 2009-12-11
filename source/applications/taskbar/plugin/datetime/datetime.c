/* Datetime plugin for the taskbar application
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

#include <time.h>
#include <stdio.h>

#include <ygui/font.h>
#include <ygui/window.h>

#include "../../plugin.h"

#define W_DATETIME W_TYPE_COUNT + 1

static font_t* datetime_font = NULL;

static int datetime_update_callback( window_t* window, void* data ) {
    widget_t* widget;

    widget = ( widget_t* )data;
    widget_invalidate( widget );

    return 0;
}

static int datetime_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    point_t position;

    static color_t bg = { 216, 216, 216, 255 };
    static color_t fg = { 0, 0, 0, 255 };

    widget_get_bounds( widget, &bounds );
    gc_set_pen_color( gc, &bg );
    gc_fill_rect( gc, &bounds );

    int asc = font_get_ascender( datetime_font );
    int desc = font_get_descender( datetime_font );
    point_init( &position, 2, ( rect_height( &bounds ) - ( asc - desc ) ) / 2 + asc );

    gc_set_font( gc, datetime_font );
    gc_set_pen_color( gc, &fg );

    time_t now = time( NULL );
    struct tm* tmp = localtime( &now );

    char time[ 8 ];
    strftime( time, sizeof( time ), "%H:%M", tmp );

    gc_draw_text( gc, &position, time, -1 );

    return 0;
}

static int datetime_get_preferred_size( widget_t* widget, point_t* size ) {
    point_init(
        size,
        font_get_string_width( datetime_font, "88:88", 5 ) + 4,
        font_get_height( datetime_font ) + 4
    );

    return 0;
}

static int datetime_added_to_window( widget_t* widget ) {
    window_insert_timer(
        widget->window,
        1 * 1000 * 1000,
        1,
        datetime_update_callback,
        ( void* )widget
    );

    return 0;
}

static widget_operations_t datetime_ops = {
    .paint = datetime_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = datetime_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = datetime_added_to_window,
    .child_added = NULL
};

static widget_t* datetime_create( void ) {
    font_properties_t properties;

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    datetime_font = create_font( "DejaVu Sans", "Book", &properties );

    if ( datetime_font == NULL ) {
        return NULL;
    }

    return create_widget( W_DATETIME, &datetime_ops, NULL );
}

taskbar_plugin_t datetime_plugin = {
    .name = "Date & time",
    .create = datetime_create
};
