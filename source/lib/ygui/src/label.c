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

#include <ygui/label.h>
#include <ygui/font.h>

typedef struct label {
    char* text;
    font_t* font;
} label_t;

static int label_paint( widget_t* widget ) {
    rect_t bounds;
    label_t* label;
    point_t text_position;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 216, 216, 216, 0xFF };

    label = ( label_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    widget_set_pen_color( widget, &bg_color );
    widget_fill_rect( widget, &bounds );

    point_init( &text_position, 5, 20 );

    widget_set_pen_color( widget, &fg_color );
    widget_set_font( widget, label->font );
    widget_draw_text( widget, &text_position, label->text, -1 );

    return 0;
}

static widget_operations_t label_ops = {
    .paint = label_paint
};

widget_t* create_label( const char* text ) {
    label_t* label;
    widget_t* widget;
    font_properties_t properties;

    label = ( label_t* )malloc( sizeof( label_t ) );

    if ( label == NULL ) {
        goto error1;
    }

    label->text = strdup( text );

    if ( label->text == NULL ) {
        goto error2;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    label->font = create_font( "DejaVu Sans", "Book", &properties );

    if ( label->font == NULL ) {
        goto error3;
    }

    widget = create_widget( W_LABEL, &label_ops, label );

    if ( widget == NULL ) {
        goto error4;
    }

    return widget;

error4:
    /* TODO: free the font */

error3:
    free( label->text );

error2:
    free( label );

error1:
    return NULL;
}
