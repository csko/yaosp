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

    gc_set_pen_color( gc, &fg_color );
    gc_draw_rect( gc, &bounds );

    rect_resize( &bounds, 1, 1, -1, -1 );

    /* Fill the background of the textarea */

    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    rect_resize( &bounds, 2, 2, -2, -2 );
    gc_translate_xy( gc, 3, 3 );

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

static string_t* _textarea_get_line( textarea_t* textarea, int index ) {
    if ( ( index < 0 ) ||
         ( index >= array_get_size( &textarea->lines ) ) ) {
        return NULL;
    }

    return ( string_t* )array_get_item( &textarea->lines, index );
}

static string_t* textarea_insert_line( textarea_t* textarea, int index ) {
    string_t* line;

    if ( ( index < 0 ) ||
         ( index > array_get_size( &textarea->lines ) ) ) {
        goto error1;
    }

    line = ( string_t* )malloc( sizeof( string_t ) );

    if ( line == NULL ) {
        goto error1;
    }

    if ( init_string( line ) != 0 ) {
        goto error2;
    }

    array_insert_item( &textarea->lines, index, ( void* )line );

    return line;

 error2:
    free( line );

 error1:
    return NULL;
}

static int textarea_remove_line( textarea_t* textarea, int index ) {
    if ( ( index < 0 ) ||
         ( index >= array_get_size( &textarea->lines ) ) ) {
        return -EINVAL;
    }

    array_remove_item_from( &textarea->lines, index );

    return 0;
}

static int textarea_key_pressed( widget_t* widget, int key ) {
    int prev_x;
    int prev_y;
    int prev_line_count;
    textarea_t* textarea;

    textarea = ( textarea_t* )widget_get_data( widget );

    prev_x = textarea->cursor_x;
    prev_y = textarea->cursor_y;
    prev_line_count = array_get_size( &textarea->lines );

    switch ( key ) {
        case KEY_BACKSPACE : {
            string_t* current;

            current = textarea_current_line( textarea );

            if ( textarea->cursor_x == 0 ) {
                if ( textarea->cursor_y > 0 ) {
                    string_t* prev;
                    int prev_size;

                    prev = _textarea_get_line( textarea, textarea->cursor_y - 1 );
                    prev_size = string_length( prev );

                    string_append_string( prev, current );

                    textarea_remove_line( textarea, textarea->cursor_y );
                    destroy_string( current );
                    free( current );

                    textarea->cursor_y--;
                    textarea->cursor_x = prev_size;
                }

                break;
            } else {
                textarea->cursor_x = string_prev_utf8_char( current, textarea->cursor_x );
                string_erase_utf8_char( current, textarea->cursor_x );
            }

            break;
        }

        case KEY_DELETE : {
            string_t* line = textarea_current_line( textarea );

            if ( textarea->cursor_x == string_length( line ) ) {
                if ( textarea->cursor_y < array_get_size( &textarea->lines ) - 1 ) {
                    string_t* next_line;

                    next_line = _textarea_get_line( textarea, textarea->cursor_y + 1 );

                    string_append_string( line, next_line );
                    destroy_string( next_line );
                    free( next_line );

                    array_remove_item_from( &textarea->lines, textarea->cursor_y + 1 );
                }
            } else {
                string_erase_utf8_char( line, textarea->cursor_x );
            }

            break;
        }

        case 10 :
        case KEY_ENTER : {
            string_t* current;
            string_t* next;
            int remaining;

            current = textarea_current_line( textarea );
            remaining = string_length( current ) - textarea->cursor_x;

            textarea->cursor_y++;

            next = textarea_insert_line( textarea, textarea->cursor_y );

            if ( remaining > 0 ) {
                string_append_from_string( next, current, textarea->cursor_x, remaining );
                string_remove( current, textarea->cursor_x, remaining );
            }

            textarea->cursor_x = 0;

            break;
        }

        case KEY_LEFT :
            if ( textarea->cursor_x > 0 ) {
                textarea->cursor_x--;
            } else {
                if ( textarea->cursor_y > 0 ) {
                    string_t* current;

                    textarea->cursor_y--;

                    current = textarea_current_line( textarea );
                    textarea->cursor_x = string_length( current );
                }
            }

            break;

        case KEY_RIGHT : {
            string_t* line = textarea_current_line( textarea );

            if ( textarea->cursor_x < string_length( line ) ) {
                textarea->cursor_x++;
            } else {
                line = _textarea_get_line( textarea, array_get_size( &textarea->lines ) - 1 );

                int last_line = string_length( line ) == 0 ?
                                array_get_size( &textarea->lines ) - 1 :
                                array_get_size( &textarea->lines );

                if ( textarea->cursor_y < last_line ) {
                    textarea->cursor_y++;
                    textarea->cursor_x = 0;

                    textarea_current_line( textarea );
                }
            }

            break;
        }

        case KEY_UP :
            if ( textarea->cursor_y > 0 ) {
                textarea->cursor_y--;
                textarea->cursor_x = 0;
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
                }
            } else {
                textarea->cursor_y++;
                textarea->cursor_x = 0;
            }

            break;

        case KEY_HOME :
            textarea->cursor_x = 0;
            break;

        case KEY_END :
            textarea->cursor_x = string_length( textarea_current_line( textarea ) );
            break;

        case KEY_PAGE_UP : {
            point_t size;
            int visible_lines;

            widget_get_visible_size( widget, &size );
            visible_lines = ( size.y - 6 ) / font_get_height( textarea->font );

            textarea->cursor_y = MAX( 0, textarea->cursor_y - visible_lines );
            textarea->cursor_x = 0;

            break;
        }

        case KEY_PAGE_DOWN : {
            point_t size;
            int last_line;
            int visible_lines;
            string_t* line;

            widget_get_visible_size( widget, &size );
            visible_lines = ( size.y - 6 ) / font_get_height( textarea->font );
            line = _textarea_get_line( textarea, array_get_size( &textarea->lines ) - 1 );

            last_line = string_length( line ) == 0 ?
                        array_get_size( &textarea->lines ) - 1 :
                        array_get_size( &textarea->lines );

            textarea->cursor_y = MIN( last_line, textarea->cursor_y + visible_lines );
            textarea->cursor_x = 0;

            textarea_current_line( textarea );

            break;
        }

        case KEY_TAB :
        case KEY_ESCAPE :
            /* these keys are not handled here ... */
            return 0;

        default : {
            string_t* line = textarea_current_line( textarea );
            string_insert( line, textarea->cursor_x++, ( char* )&key, 1 );
            break;
        }
    }

    /* Check if the viewport size is changed */

    if ( ( prev_x != textarea->cursor_x ) ||
         ( prev_y != textarea->cursor_y ) ) {
        widget_signal_event_handler(
            widget,
            widget->event_ids[ E_VIEWPORT_CHANGED ]
        );
    }

    /* Check if the preferred size is changed */

    if ( prev_line_count != array_get_size( &textarea->lines ) ) {
        widget_signal_event_handler(
            widget,
            widget->event_ids[ E_PREF_SIZE_CHANGED ]
        );
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
        array_get_size( &textarea->lines ) * font_get_height( textarea->font ) + 6
    );

    return 0;
}

static int textarea_get_viewport( widget_t* widget, rect_t* viewport ) {
    textarea_t* textarea;

    textarea = ( textarea_t* )widget_get_data( widget );

    rect_init(
        viewport,
        3, textarea->cursor_y * font_get_height( textarea->font ) + 3,
        0, ( textarea->cursor_y + 1 ) * font_get_height( textarea->font ) - 1 + 3
    );

    return 0;
}

static int textarea_destroy( widget_t* widget ) {
    int i;
    int size;
    textarea_t* textarea;

    textarea = ( textarea_t* )widget_get_data( widget );

    size = array_get_size( &textarea->lines );

    for ( i = 0; i < size; i++ ) {
        string_t* line;

        line = ( string_t* )array_get_item( &textarea->lines, i );

        destroy_string( line );
        free( line );
    }

    destroy_array( &textarea->lines );
    destroy_font( textarea->font );
    free( textarea );

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
    .child_added = NULL,
    .destroy = textarea_destroy
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

    widget = create_widget( W_TEXTAREA, WIDGET_FOCUSABLE, &textarea_ops, textarea );

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

int textarea_get_line_count( widget_t* widget ) {
    textarea_t* textarea;

    if ( ( widget == NULL ) ||
         ( widget_get_id( widget ) != W_TEXTAREA ) ) {
        return 0;
    }

    textarea = ( textarea_t* )widget_get_data( widget );

    return array_get_size( &textarea->lines );
}

char* textarea_get_line( widget_t* widget, int index ) {
    textarea_t* textarea;

    if ( ( widget == NULL ) ||
         ( widget_get_id( widget ) != W_TEXTAREA )||
         ( index < 0 ) ) {
        return NULL;
    }

    textarea = ( textarea_t* )widget_get_data( widget );

    if ( index >= array_get_size( &textarea->lines ) ) {
        return NULL;
    }

    string_t* line = ( string_t* )array_get_item( &textarea->lines, index );

    return string_dup_buffer( line );
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
