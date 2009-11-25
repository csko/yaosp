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
#include <yaosp/input.h>

#include <ygui/textfield.h>
#include <ygui/yconstants.h>

#include <yutil/string.h>
#include <yutil/macros.h>
#include <yutil/array.h>

typedef struct textarea {
    font_t* font;

    array_t lines;
} textarea_t;

static int textarea_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    textarea_t* textarea;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 255, 255, 255, 0xFF };

    textarea = ( textarea_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Draw the border of the textarea */

    gc_set_pen_color( gc, &fg_color );
    gc_draw_rect( gc, &bounds );

    /* Fill the background of the textarea */

    rect_resize( &bounds, 1, 1, -1, -1 );
    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    return 0;
}

static int textarea_get_preferred_size( widget_t* widget, point_t* size ) {
    textarea_t* textarea;

    textarea = ( textarea_t* )widget_get_data( widget );

    point_init(
        size,
        -1,
        font_get_height( textarea->font ) + 6
    );

    return 0;
}

static widget_operations_t textarea_ops = {
    .paint = textarea_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = textarea_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL
};

widget_t* create_textarea( void ) {
    widget_t* widget;
    textarea_t* textarea;
    font_properties_t properties;

    textarea = ( textarea_t* )calloc( 1, sizeof( textarea_t ) );

    if ( textarea == NULL ) {
        goto error1;
    }

    if ( init_array( &textarea->lines ) != 0 ) {
        goto error2;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    textarea->font = create_font( "DejaVu Sans", "Book", &properties );

    if ( textarea->font == NULL ) {
        goto error3;
    }

    widget = create_widget( W_TEXTAREA, &textarea_ops, textarea );

    if ( widget == NULL ) {
        goto error4;
    }

    return widget;

 error4:
    /* TODO: destroy the font */

 error3:
    destroy_array( &textarea->lines );

 error2:
    free( textarea );

 error1:
    return NULL;
}
