/* Terminal application
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
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/param.h>

#include <yaosp/input.h>

#include "term_widget.h"

#define W_TERMINAL W_TYPE_COUNT

extern int master_pty;
extern terminal_t* terminal;

static color_t terminal_color_table[ T_COLOR_COUNT ] = {
    {   0,   0,   0, 255 }, /* black */
    { 255,   0,   0, 255 }, /* red */
    {   0, 255,   0, 255 }, /* green */
    { 255, 255,   0, 255 }, /* yellow */
    {   0,   0, 255, 255 }, /* blue */
    { 255,   0, 255, 255 }, /* magenta */
    {   0, 255, 255, 255 }, /* cyan */
    { 255, 255, 255, 255 }  /* white */
};

static inline int terminal_paint_line_part( terminal_widget_t* term_widget, gc_t* gc, terminal_line_t* term_line,
                                     point_t* pos, int start, int end ) {
    int width;
    int length;
    terminal_attr_t* attr;

    assert( start <= end );

    attr = &term_line->attr[ start ];
    length = end - start + 1;
    width = font_get_string_width( term_widget->font, term_line->buffer + start, length );

    if ( attr->bg_color != T_COLOR_BLACK ) {
        rect_t tmp;

        rect_init(
            &tmp,
            pos->x,
            pos->y - font_get_ascender( term_widget->font ),
            pos->x + width - 1,
            pos->y - font_get_descender( term_widget->font ) + font_get_line_gap( term_widget->font ) - 1
        );

        gc_set_pen_color( gc, &terminal_color_table[ attr->bg_color ] );
        gc_fill_rect( gc, &tmp );
    }

    gc_set_pen_color( gc, &terminal_color_table[ attr->fg_color ] );
    gc_draw_text( gc, pos, term_line->buffer + start, length );

    pos->x += width;

    return 0;
}

static int terminal_paint_line( terminal_widget_t* terminal_widget, gc_t* gc, terminal_line_t* term_line,
                                point_t* pos, int cursor_x ) {
    int i, index;

    pos->x = 0;
    index = 0;

    while ( index < term_line->size ) {
        terminal_attr_t* attr;
        terminal_attr_t current_attr;

        terminal_attr_copy( &current_attr, &term_line->attr[ index ] );

        for ( i = index, attr = &term_line->attr[ index ]; i < term_line->size; i++, attr++ ) {
            if ( terminal_attr_compare( &current_attr, attr ) != 0 ) {
                break;
            }
        }

        terminal_paint_line_part(
            terminal_widget,
            gc,
            term_line,
            pos,
            index,
            i - 1
        );

        index = i;
    }

    if ( cursor_x != -1 ) {
        int char_width;
        point_t cursor_pos;
        rect_t cursor_rect;

        char_width = font_get_string_width( terminal_widget->font, "A", 1 );

        rect_init(
            &cursor_rect,
            cursor_x * char_width,
            pos->y - font_get_ascender( terminal_widget->font ),
            ( cursor_x + 1 ) * char_width - 1,
            pos->y - font_get_descender( terminal_widget->font ) + font_get_line_gap( terminal_widget->font ) - 1
        );

        gc_set_pen_color( gc, &terminal_color_table[ term_line->attr[ cursor_x ].fg_color ] );
        gc_fill_rect( gc, &cursor_rect );

        point_init(
            &cursor_pos,
            char_width * cursor_x,
            pos->y
        );

        gc_set_pen_color( gc, &terminal_color_table[ term_line->attr[ cursor_x ].bg_color ] );
        gc_draw_text( gc, &cursor_pos, term_line->buffer + cursor_x, 1 );
    }

    return 0;
}

static int terminal_paint( widget_t* widget, gc_t* gc ) {
    int i;
    rect_t bounds;
    int line_height;
    int history_size;
    terminal_line_t* term_line;
    terminal_widget_t* terminal_widget;

    static color_t bg_color = { 0, 0, 0, 255 };

    terminal_widget = ( terminal_widget_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Fill the background */

    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    /* Set the terminal font */

    gc_set_font( gc, terminal_widget->font );
    line_height = font_get_height( terminal_widget->font );

    point_t pos = {
        .x = 0,
        .y = font_get_ascender( terminal_widget->font )
    };

    /* Draw the visible lines to the terminal */

    pthread_mutex_lock( &terminal->lock );

    int line_count = terminal_buffer_get_line_count( &terminal->buffer );

    if ( line_count == 0 ) {
        goto paint_done;
    }

    point_t offset;
    widget_get_scroll_offset( widget, &offset );

    int first_line = -offset.y / line_height;
    int last_line = first_line + ( rect_height( &bounds ) + line_height - 1 ) / line_height;

    assert( first_line < line_count );
    last_line = MIN( last_line, line_count - 1 );
    assert( first_line <= last_line );

    pos.y += first_line * line_height;
    history_size = terminal_buffer_get_history_size( &terminal->buffer );

    for ( i = first_line; i <= last_line; i++ ) {
        term_line = terminal_buffer_get_line_at( &terminal->buffer, i );

        terminal_paint_line(
            terminal_widget,
            gc,
            term_line,
            &pos,
            ( i == history_size + terminal->buffer.cursor_y ) ? terminal->buffer.cursor_x : -1
        );

        pos.y += line_height;
    }

 paint_done:
    pthread_mutex_unlock( &terminal->lock );

    return 0;
}

static int terminal_key_pressed( widget_t* widget, int key ) {
    switch ( key ) {
        case KEY_UP :
            write( master_pty, "\x1b[A", 3 );
            break;

        case KEY_DOWN :
            write( master_pty, "\x1b[B", 3 );
            break;

        case KEY_LEFT :
            write( master_pty, "\x1b[D", 3 );
            break;

        case KEY_RIGHT :
            write( master_pty, "\x1b[C", 3 );
            break;

        case KEY_HOME :
            write( master_pty, "\x1b[H", 3 );
            break;

        case KEY_END :
            write( master_pty, "\x1b[F", 3 );
            break;

        case KEY_DELETE :
            write( master_pty, "\x1b[3~", 4 );

#if 0
            terminal_buffer_dump(
                &( ( terminal_widget_t* )widget_get_data( widget ) )->terminal->buffer
            );
#endif

            break;

        case KEY_PAGE_UP :
            write( master_pty, "\x1b[5~", 4 );
            break;

        case KEY_PAGE_DOWN :
            write( master_pty, "\x1b[6~", 4 );
            break;

        default :
            if ( key < 256 ) {
                write( master_pty, &key, 1 );
            }

            break;
    }

    return 0;
}

static int terminal_get_preferred_size( widget_t* widget, point_t* size ) {
    terminal_widget_t* terminal_widget;

    terminal_widget = ( terminal_widget_t* )widget_get_data( widget );

    point_init(
        size,
        font_get_string_width( terminal_widget->font, "A", 1 ) * 80,
        font_get_height( terminal_widget->font ) * MAX( 25, terminal_buffer_get_line_count( &terminal->buffer ) )
    );

    return 0;
}

int terminal_widget_get_character_size( widget_t* widget, int* width, int* height ) {
    terminal_widget_t* terminal_widget;

    if ( widget_get_id( widget ) != W_TERMINAL ) {
        return -EINVAL;
    }

    terminal_widget = ( terminal_widget_t* )widget_get_data( widget );

    if ( width != NULL ) {
        *width = font_get_string_width( terminal_widget->font, "A", 1 );
    }

    if ( height != NULL ) {
        *height = font_get_height( terminal_widget->font );
    }

    return 0;
}

static widget_operations_t terminal_ops = {
    .paint = terminal_paint,
    .key_pressed = terminal_key_pressed,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = terminal_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = NULL,
    .child_added = NULL
};

widget_t* create_terminal_widget( terminal_t* terminal ) {
    widget_t* widget;
    terminal_widget_t* terminal_widget;
    font_properties_t properties;

    terminal_widget = ( terminal_widget_t* )malloc( sizeof( terminal_widget_t ) );

    if ( terminal_widget == NULL ) {
        goto error1;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    terminal_widget->font = create_font( "DejaVu Sans Mono", "Book", &properties );

    if ( terminal_widget->font == NULL ) {
        goto error2;
    }

    widget = create_widget( W_TERMINAL, &terminal_ops, terminal_widget );

    if ( widget == NULL ) {
        goto error3;
    }

    terminal_widget->terminal = terminal;

    return widget;

 error3:
    /* TODO: destroy the font */

 error2:
    free( terminal );

 error1:
    return NULL;
}
