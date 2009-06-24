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
    point_t text_position;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 216, 216, 216, 0xFF };
    static color_t pressed_bg_color = { 116, 116, 166, 0xFF };

    button = ( button_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Draw the border of the button */

    widget_set_pen_color( widget, &fg_color );
    widget_draw_rect( widget, &bounds );

    /* Fill the background of the button */

    if ( button->pressed ) {
        widget_set_pen_color( widget, &pressed_bg_color );
    } else {
        widget_set_pen_color( widget, &bg_color );
    }

    rect_resize( &bounds, 1, 1, -1, -1 );

    widget_fill_rect( widget, &bounds );

    /* Draw the text to the button */

    switch ( button->h_align ) {
        case H_ALIGN_LEFT :
            text_position.x = bounds.left;
            break;

        case H_ALIGN_CENTER : {
            int text_width;

            text_width = font_get_string_width( button->font, button->text, -1 );

            text_position.x = bounds.left + ( rect_width( &bounds ) - text_width ) / 2;

            if ( text_position.x < bounds.left ) {
                text_position.x = bounds.left;
            }

            break;
        }

        case H_ALIGN_RIGHT : {
            int text_width;

            text_width = font_get_string_width( button->font, button->text, -1 );

            text_position.x = bounds.right - text_width + 1;

            if ( text_position.x < bounds.left ) {
                text_position.x = bounds.left;
            }

            break;
        }
    }

    switch ( button->v_align ) {
        case V_ALIGN_TOP :
            text_position.y = bounds.top + font_get_ascender( button->font );
            break;

        case V_ALIGN_CENTER : {
            int asc = font_get_ascender( button->font );
            int desc = font_get_descender( button->font );

            text_position.y = bounds.top + ( rect_height( &bounds ) - ( asc - desc ) ) / 2 + asc;

            break;
        }

        case V_ALIGN_BOTTOM :
            text_position.y = bounds.bottom - ( font_get_line_gap( button->font ) - font_get_descender( button->font ) );
            break;
    }

    widget_set_pen_color( widget, &fg_color );
    widget_set_font( widget, button->font );
    widget_set_clip_rect( widget, &bounds );
    widget_draw_text( widget, &text_position, button->text, -1 );

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

static int button_get_preferred_size( widget_t* widget, point_t* size ) {
    button_t* button;

    button = ( button_t* )widget_get_data( widget );

    point_init(
        size,
        font_get_string_width( button->font, button->text, -1 ) + 6,
        font_get_ascender( button->font ) - font_get_descender( button->font ) + font_get_line_gap( button->font ) + 6
    );

    return 0;
}

static widget_operations_t button_ops = {
    .paint = button_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = button_pressed,
    .mouse_released = button_released,
    .get_minimum_size = NULL,
    .get_preferred_size = button_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL
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


