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

enum {
    E_ACTIVATED,
    E_COUNT
};

typedef struct textfield {
    font_t* font;

    string_t buffer;

    int text_start;
    int cursor_start;

    int left_offset;

    v_alignment_t v_align;
} textfield_t;

static int textfield_events[ E_COUNT ] = {
    -1
};

static event_type_t textfield_event_types[ E_COUNT ] = {
    { "activated", &textfield_events[ E_ACTIVATED ] }
};

static int textfield_paint( widget_t* widget, gc_t* gc ) {
    rect_t bounds;
    point_t text_position;
    textfield_t* textfield;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 255, 255, 255, 0xFF };

    textfield = ( textfield_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Draw the border of the textfield */

    gc_set_pen_color( gc, &fg_color );
    gc_draw_rect( gc, &bounds );

    /* Fill the background of the textfield */

    rect_resize( &bounds, 1, 1, -1, -1 );
    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    /* Calculate the text position */

    text_position.x = bounds.left;

    switch ( textfield->v_align ) {
        case V_ALIGN_TOP :
            text_position.y = bounds.top + font_get_ascender( textfield->font );
            break;

        case V_ALIGN_CENTER : {
            int asc = font_get_ascender( textfield->font );
            int desc = font_get_descender( textfield->font );

            text_position.y = bounds.top + ( rect_height( &bounds ) - ( asc - desc ) ) / 2 + asc;

            break;
        }

        case V_ALIGN_BOTTOM :
            text_position.y = bounds.bottom - ( font_get_line_gap( textfield->font ) - font_get_descender( textfield->font ) );
            break;
    }

    /* Draw the text to the widget */

    gc_set_pen_color( gc, &fg_color );
    gc_set_font( gc, textfield->font );
    gc_set_clip_rect( gc, &bounds );

    gc_draw_text(
        gc,
        &text_position,
        string_c_str( &textfield->buffer ) + textfield->text_start,
        -1
    );

    return 0;
}

static int textfield_key_pressed( widget_t* widget, int key ) {
    textfield_t* textfield;

    textfield = ( textfield_t* )widget_get_data( widget );

    switch ( key ) {
        case 10 :
        case KEY_ENTER :
            widget_signal_event_handler( widget, textfield_events[ E_ACTIVATED ] );

            break;

        case KEY_TAB :
            break;

        case KEY_DELETE :
            break;

        case KEY_BACKSPACE :
            if ( textfield->cursor_start > 0 ) {
                textfield->cursor_start -= 1;

                string_remove( &textfield->buffer, textfield->cursor_start, 1 );
            }

            break;

        case KEY_LEFT :
            if ( textfield->cursor_start > 0 ) {
                textfield->cursor_start -= 1;
            }

            break;

        case KEY_RIGHT :
            if ( textfield->cursor_start < string_length( &textfield->buffer ) ) {
                textfield->cursor_start += 1;
            }

            break;

        case KEY_UP :
            break;

        case KEY_DOWN :
            break;

        default :
            string_insert( &textfield->buffer, textfield->cursor_start, ( char* )&key, 1 );
            textfield->cursor_start += 1;

            break;
    }

    widget_invalidate( widget );

    return 0;
}

static int textfield_get_preferred_size( widget_t* widget, point_t* size ) {
    textfield_t* textfield;

    textfield = ( textfield_t* )widget_get_data( widget );

    point_init(
        size,
        -1,
        font_get_height( textfield->font ) + 6
    );

    return 0;
}

static widget_operations_t textfield_ops = {
    .paint = textfield_paint,
    .key_pressed = textfield_key_pressed,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = textfield_get_preferred_size,
    .get_maximum_size = NULL,
    .get_viewport = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL
};

char* textfield_get_text( widget_t* widget ) {
    textfield_t* textfield;

    y_return_val_if_fail( IS_TEXTFIELD( widget ), NULL );

    textfield = TEXTFIELD( widget );

    return string_dup_buffer( &textfield->buffer );
}

int textfield_set_text( widget_t* widget, char* text ) {
    textfield_t* textfield;

    y_return_val_if_fail( IS_TEXTFIELD( widget ), -EINVAL );

    textfield = TEXTFIELD( widget );

    string_clear( &textfield->buffer );

    textfield->text_start = 0;
    textfield->cursor_start = 0;

    if ( text != NULL ) {
        string_append( &textfield->buffer, text, strlen( text ) );
    }

    widget_invalidate( widget );

    return 0;
}

widget_t* create_textfield( void ) {
    widget_t* widget;
    textfield_t* textfield;
    font_properties_t properties;

    textfield = ( textfield_t* )calloc( 1, sizeof( textfield_t ) );

    if ( textfield == NULL ) {
        goto error1;
    }

    if ( init_string( &textfield->buffer ) != 0 ) {
        goto error2;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    textfield->font = create_font( "DejaVu Sans", "Book", &properties );

    if ( textfield->font == NULL ) {
        goto error3;
    }

    widget = create_widget( W_TEXTFIELD, &textfield_ops, textfield );

    if ( widget == NULL ) {
        goto error4;
    }

    if ( widget_add_events( widget, textfield_event_types, textfield_events, E_COUNT ) != 0 ) {
        goto error5;
    }

    textfield->v_align = V_ALIGN_CENTER;

    return widget;

 error5:
    /* TODO: destroy the widget */

 error4:
    destroy_font( textfield->font );

 error3:
    destroy_string( &textfield->buffer );

 error2:
    free( textfield );

 error1:
    return NULL;
}
