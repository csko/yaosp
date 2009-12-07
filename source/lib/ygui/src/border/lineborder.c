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

#include <ygui/widget.h>
#include <ygui/border/lineborder.h>

static color_t black = { 0, 0, 0, 255 };
static color_t bg = { 216, 216, 216, 255 };

static int line_border_paint( widget_t* widget, gc_t* gc ) {
    rect_t tmp;
    point_t size;

    widget_get_size( widget, &size );

    /* Paint the black border */

    gc_set_pen_color( gc, &black );

    rect_init( &tmp, 0, 0, size.x - 1, size.y - 1 );
    gc_draw_rect( gc, &tmp );

    /* Paint the spacing between the border and the widget */

    gc_set_pen_color( gc, &bg );

    /* top - */
    rect_init( &tmp, 1, 1, size.x - 2, 2 );
    gc_fill_rect( gc, &tmp );
    /* bottom - */
    rect_init( &tmp, 1, size.y - 3, size.x - 2, size.y - 2 );
    gc_fill_rect( gc, &tmp );
    /* left | */
    rect_init( &tmp, 1, 1, 2, size.y - 2 );
    gc_fill_rect( gc, &tmp );
    /* right | */
    rect_init( &tmp, size.x - 3, 1, size.x - 2, size.y - 2 );
    gc_fill_rect( gc, &tmp );

    return 0;
}

static border_operations_t line_border_ops = {
    .paint = line_border_paint
};

border_t* create_line_border( void ) {
    border_t* border;

    border = create_border( &line_border_ops );

    if ( border == NULL ) {
        return NULL;
    }

    point_init( &border->size, 6, 6 );
    point_init( &border->lefttop_offset, 3, 3 );

    return border;
}
