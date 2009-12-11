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
#include <assert.h>
#include <yaosp/input.h>

#include <ygui/textfield.h>
#include <ygui/yconstants.h>

#include <yutil/string.h>
#include <yutil/macros.h>
#include <yutil/array.h>

typedef struct textarea {
    font_t* font;

    array_t lines;

    int cursor_x;
    int cursor_y;
} textarea_t;

static inline int textarea_paint_line_with_cursor( textarea_t* textarea, gc_t* gc, string_t* line,
                                                   point_t* position, int cursor_x ) {
    int width;
    rect_t tmp;
    const char* data;

    data = string_c_str( line );
    width = font_get_string_width( textarea->font, data, cursor_x );

    gc_draw_text( gc, position, data, cursor_x );

    rect_init(
        &tmp,
        width,
        position->y - font_get_ascender( textarea->font ) + 1,
        width,
        position->y - font_get_descender( textarea->font ) + font_get_line_gap( textarea->font ) - 1
    );

    gc_fill_rect( gc, &tmp );

    position->x += width;
    gc_draw_text( gc, position, data + cursor_x, -1 );
    position->x -= width;

    return 0;
}

static inline int textarea_paint_line_without_cursor( gc_t* gc, string_t* line, point_t* position ) {
    gc_draw_text( gc, position, string_c_str( line ), -1 );

    return 0;
}

static int textarea_paint( widget_t* widget, gc_t* gc ) {
    int i;
    int lines;
    rect_t bounds;
    textarea_t* textarea;

    static color_t fg_color = { 0, 0, 0, 255 };
    static color_t bg_color = { 255, 255, 255, 255 };

    textarea = ( textarea_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Fill the background of the textarea */

    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    /* Draw the lines */

    gc_set_font( gc, textarea->font );
    gc_set_pen_color( gc, &fg_color );

    lines = array_get_size( &textarea->lines );

    point_t position = {
        .x = 0,
        .y = font_get_ascender( textarea->font )
    };
    int line_height = font_get_height( textarea->font );

    for ( i = 0; i < lines; i++ ) {
        string_t* line = ( string_t* )array_get_item( &textarea->lines, i );

        if ( i == textarea->cursor_y ) {
            textarea_paint_line_with_cursor( textarea, gc, line, &position, textarea->cursor_x );
        } else {
            textarea_paint_line_without_cursor( gc, line, &position );
        }

        position.y += line_height;
    }

    return 0;
}

static string_t* textarea_current_line( textarea_t* textarea ) {
    int lines;
    string_t* line;

    lines = array_get_size( &textarea->lines );

    assert( ( textarea->cursor_y >= 0 ) && ( textarea->cursor_y <= lines ) );

    if ( textarea->cursor_y == lines ) {
        line = ( string_t* )malloc( sizeof( string_t ) );

        if ( line == NULL ) {
            goto out;
        }

        if ( init_string( line ) != 0 ) {
            free( line );
            line = NULL;

            goto out;
        }

        array_add_item( &textarea->lines, ( void* )line );
    } else {
        line = ( string_t* )array_get_item( &textarea->lines, textarea->cursor_y );
    }

 out:
    return line;
}

static int textarea_key_pressed( widget_t* widget, int key ) {
    textarea_t* textarea;

    textarea = ( textarea_t* )widget_get_data( widget );

    switch ( key ) {
        case KEY_BACKSPACE : {
            if ( textarea->cursor_x == 0 ) {
                break;
            }

            string_t* line = textarea_current_line( textarea );
            textarea->cursor_x = string_prev_utf8_char( line, textarea->cursor_x );

            string_erase_utf8_char( line, textarea->cursor_x );

            break;
        }

        case KEY_DELETE : {
            string_t* line = textarea_current_line( textarea );
            string_erase_utf8_char( line, textarea->cursor_x );

            break;
        }

        case KEY_TAB :
            break;

        case KEY_ESCAPE :
            break;

        case 10 :
        case KEY_ENTER :
            textarea->cursor_y++;
            textarea->cursor_x = 0;

            textarea_current_line( textarea );

            widget_signal_event_handler(
                widget,
                widget->event_ids[ E_PREF_SIZE_CHANGED ]
            );

            widget_signal_event_handler(
                widget,
                widget->event_ids[ E_VIEWPORT_CHANGED ]
            );

            break;

        case KEY_LEFT :
            if ( textarea->cursor_x > 0 ) {
                textarea->cursor_x--;

                widget_signal_event_handler(
                    widget,
                    widget->event_ids[ E_VIEWPORT_CHANGED ]
                );
            }

            break;

        case KEY_RIGHT : {
            string_t* line = textarea_current_line( textarea );

            if ( textarea->cursor_x < string_length( line ) ) {
                textarea->cursor_x++;

                widget_signal_event_handler(
                    widget,
                    widget->event_ids[ E_VIEWPORT_CHANGED ]
                );
            }

            break;
        }

        case KEY_UP :
            if ( textarea->cursor_y > 0 ) {
                textarea->cursor_y--;

                widget_signal_event_handler(
                    widget,
                    widget->event_ids[ E_VIEWPORT_CHANGED ]
                );
            }

            break;

        case KEY_DOWN :
            assert( textarea->cursor_y < array_get_size( &textarea->lines ) );

            if ( textarea->cursor_y == array_get_size( &textarea->lines ) - 1 ) {
                string_t* line = textarea_current_line( textarea );

                if ( string_length( line ) > 0 ) {
                    textarea->cursor_y++;
                    textarea->cursor_x = 0;

                    textarea_current_line( textarea );

                    widget_signal_event_handler(
                        widget,
                        widget->event_ids[ E_PREF_SIZE_CHANGED ]
                    );
                }
            } else {
                textarea->cursor_y++;
                textarea->cursor_x = 0;
            }

            widget_signal_event_handler(
                widget,
                widget->event_ids[ E_VIEWPORT_CHANGED ]
            );

            break;

        default : {
            string_t* line = textarea_current_line( textarea );
            string_insert( line, textarea->cursor_x++, ( char* )&key, 1 );
            break;
        }
    }

    widget_invalidate( widget );

    return 0;
}

static int textarea_get_preferred_size( widget_t* widget, point_t* size ) {
    textarea_t* textarea;

    textarea = ( textarea_t* )widget_get_data( widget );

    point_init(
        size,
        450 /* todo */,
        array_get_size( &textarea->lines ) * font_get_height( textarea->font )
    );

    return 0;
}

static int textarea_get_viewport( widget_t* widget, rect_t* viewport ) {
    textarea_t* textarea;

    textarea = ( textarea_t* )widget_get_data( widget );

    rect_init(
        viewport,
        0, textarea->cursor_y * font_get_height( textarea->font ),
        0, ( textarea->cursor_y + 1 ) * font_get_height( textarea->font ) - 1
    );

    return 0;
}

static widget_operations_t textarea_ops = {
    .paint = textarea_paint,
    .key_pressed = textarea_key_pressed,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = textarea_get_preferred_size,
    .get_maximum_size = NULL,
    .get_viewport = textarea_get_viewport,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL
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

    textarea_current_line( textarea );

    return widget;

 error4:
    destroy_font( textarea->font );

 error3:
    destroy_array( &textarea->lines );

 error2:
    free( textarea );

 error1:
    return NULL;
}

int textarea_add_lines( widget_t* widget, array_t* lines ) {
    textarea_t* textarea;

    if ( ( widget == NULL ) ||
         ( lines == NULL ) ||
         ( widget_get_id( widget ) != W_TEXTAREA ) ) {
        return -EINVAL;
    }

    textarea = ( textarea_t* )widget_get_data( widget );

    array_add_items( &textarea->lines, lines );

    /* The preferred size of the widget is just changed ... */

    widget_signal_event_handler(
        widget,
        widget->event_ids[ E_PREF_SIZE_CHANGED ]
    );

    return 0;
}

int textarea_set_lines( widget_t* widget, array_t* lines ) {
    int i;
    int size;
    textarea_t* textarea;

    if ( ( widget == NULL ) ||
         ( lines == NULL ) ||
         ( widget_get_id( widget ) != W_TEXTAREA ) ) {
        return -EINVAL;
    }

    textarea = ( textarea_t* )widget_get_data( widget );

    size = array_get_size( &textarea->lines );

    for ( i = 0; i < size; i++ ) {
        string_t* line;

        line = ( string_t* )array_get_item( &textarea->lines, i );

        destroy_string( line );
        free( line );
    }

    array_make_empty( &textarea->lines );
    array_add_items( &textarea->lines, lines );

    /* The preferred size of the widget is just changed ... */

    widget_signal_event_handler(
        widget,
        widget->event_ids[ E_PREF_SIZE_CHANGED ]
    );

    return 0;
}
