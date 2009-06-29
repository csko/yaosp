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

static int terminal_paint_line( terminal_widget_t* terminal_widget, gc_t* gc, terminal_line_t* term_line, point_t* pos ) {
    int j;
    int index;

    pos->x = 0;
    index = 0;

    while ( index < term_line->size ) {
        terminal_color_item_t* color_item;

        for ( j = index, color_item = &term_line->buffer_color[ index ]; j < term_line->size; j++, color_item++ ) {
            if ( ( color_item->bg_color != terminal_widget->current_bg_color ) ||
                 ( color_item->fg_color != terminal_widget->current_fg_color ) ) {
                break;
            }
        }

        if ( j > index ) {
            int width;

            width = font_get_string_width( terminal_widget->font, term_line->buffer + index, j - index );

            if ( color_item->bg_color != T_COLOR_BLACK ) {
                rect_t tmp;

                gc_set_pen_color( gc, &terminal_color_table[ terminal_widget->current_bg_color ] );

                rect_init(
                    &tmp,
                    pos->x,
                    pos->y - font_get_ascender( terminal_widget->font ),
                    pos->x + width - 1,
                    pos->y - font_get_descender( terminal_widget->font ) + font_get_line_gap( terminal_widget->font ) - 1
                );

                gc_fill_rect( gc, &tmp );
            }

            gc_set_pen_color( gc, &terminal_color_table[ terminal_widget->current_fg_color ] );
            gc_draw_text( gc, pos, term_line->buffer + index, j - index );

            pos->x += width;
        }

        index = j;

        terminal_widget->current_bg_color = color_item->bg_color;
        terminal_widget->current_fg_color = color_item->fg_color;
    }

    return 0;
}

static int terminal_paint( widget_t* widget, gc_t* gc ) {
    int i;
    rect_t bounds;
    terminal_line_t* term_line;
    terminal_widget_t* terminal_widget;

    /* 190, 190, 190 */

    static color_t bg_color = { 0, 0, 0, 255 };
    static color_t fg_color = { 255, 255, 255, 255 };

    terminal_widget = ( terminal_widget_t* )widget_get_data( widget );

    widget_get_bounds( widget, &bounds );

    /* Fill the background */

    gc_set_pen_color( gc, &bg_color );
    gc_fill_rect( gc, &bounds );

    /* Set the terminal font */

    gc_set_font( gc, terminal_widget->font );

    point_t pos = { .x = 0, .y = font_get_ascender( terminal_widget->font ) };

    gc_set_pen_color( gc, &fg_color );

    terminal_widget->current_bg_color = T_COLOR_BLACK;
    terminal_widget->current_fg_color = T_COLOR_WHITE;

    LOCK( terminal->lock );

    for ( i = 0, term_line = terminal->lines; i < terminal->max_lines; i++, term_line++ ) {
        terminal_paint_line( terminal_widget, gc, term_line, &pos );

        pos.y += font_get_height( terminal_widget->font );
    }

    UNLOCK( terminal->lock );

    return 0;
}

static int terminal_key_pressed( widget_t* widget, int key ) {
    if ( key < 256 ) {
        write( master_pty, &key, 1 );
    }

    return 0;
}

static int terminal_get_preferred_size( widget_t* widget, point_t* size ) {
    terminal_widget_t* terminal_widget;

    terminal_widget = ( terminal_widget_t* )widget_get_data( widget );

    point_init(
        size,
        font_get_string_width( terminal_widget->font, "A", 1 ) * 80,
        font_get_height( terminal_widget->font ) * MAX( 25, ( terminal_widget->terminal->last_line + 1 ) )
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
    .size_changed = NULL
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
