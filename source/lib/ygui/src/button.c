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

#include <ygui/button.h>
#include <ygui/yconstants.h>

enum {
    E_CLICKED,
    E_COUNT
};

typedef struct button {
    char* text;
    font_t* font;
    h_alignment_t h_align;
    v_alignment_t v_align;
    int pressed;
} button_t;

static event_type_t button_event_types[ E_COUNT ] = {
    { "clicked" }
};

static int button_events[ E_COUNT ] = {
    0
};

static int button_paint( widget_t* widget ) {
    rect_t bounds;
    button_t* button;

    //static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 216, 216, 216, 0xFF };
    static color_t pressed_bg_color = { 116, 116, 166, 0xFF };

    button = ( button_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    if ( button->pressed ) {
        widget_set_pen_color( widget, &pressed_bg_color );
    } else {
        widget_set_pen_color( widget, &bg_color );
    }

    widget_fill_rect( widget, &bounds );

    return 0;
}

static int button_pressed( widget_t* widget, point_t* pos, int mouse_button ) {
    button_t* button;

    button = ( button_t* )widget_get_data( widget );
    button->pressed = 1;

    widget_invalidate( widget, 1 );

    return 0;
}

static int button_released( widget_t* widget, int mouse_button ) {
    button_t* button;

    button = ( button_t* )widget_get_data( widget );
    button->pressed = 0;

    widget_invalidate( widget, 1 );

    widget_signal_event_handler( widget, button_events[ E_CLICKED ] );

    return 0;
}

static widget_operations_t button_ops = {
    .paint = button_paint,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = button_pressed,
    .mouse_released = button_released
};

widget_t* create_button( const char* text ) {
    int error;
    button_t* button;
    widget_t* widget;
    font_properties_t properties;

    button = ( button_t* )malloc( sizeof( button_t ) );

    if ( button == NULL ) {
        goto error1;
    }

    button->text = strdup( text );

    if ( button->text == NULL ) {
        goto error2;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    button->font = create_font( "DejaVu Sans", "Book", &properties );

    if ( button->font == NULL ) {
        goto error3;
    }

    widget = create_widget( W_BUTTON, &button_ops, button );

    if ( widget == NULL ) {
        goto error4;
    }

    error = widget_set_events( widget, button_event_types, button_events, E_COUNT );

    if ( error < 0 ) {
        goto error5;
    }

    button->h_align = H_ALIGN_CENTER;
    button->v_align = V_ALIGN_CENTER;
    button->pressed = 0;

    return widget;

error5:
    /* TODO: destroy the widget */

error4:
    /* TODO: free the font */

error3:
    free( button->text );

error2:
    free( button );

error1:
    return NULL;
}


