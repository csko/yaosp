/* yaosp GUI library
 *
 * Copyright (c) 2010 Zoltan Kovacs
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
#include <ygui/border/emptyborder.h>

typedef struct emptyborder {
    int left;
    int top;
    int right;
    int bottom;
} emptyborder_t;

static color_t bg = { 216, 216, 216, 255 };

static int empty_border_paint( widget_t* widget, gc_t* gc ) {
    rect_t tmp;
    point_t size;
    border_t* border;
    emptyborder_t* emptyborder;

    border = widget_get_border(widget);
    emptyborder = (emptyborder_t*)border_get_data(border);

    widget_get_full_size( widget, &size );

    /* Fill the spacing between the border and the widget */

    gc_set_pen_color( gc, &bg );

    /* top - */
    rect_init( &tmp, 0, 0, size.x - 1, emptyborder->top - 1 );
    gc_fill_rect( gc, &tmp );
    /* bottom - */
    rect_init( &tmp, 0, size.y - emptyborder->bottom, size.x - 1, size.y - 1 );
    gc_fill_rect( gc, &tmp );
    /* left | */
    rect_init( &tmp, 0, 0, emptyborder->left - 1, size.y - 1 );
    gc_fill_rect( gc, &tmp );
    /* right | */
    rect_init( &tmp, size.x - emptyborder->right, 0, size.x - 1, size.y - 1 );
    gc_fill_rect( gc, &tmp );

    border_dec_ref(border);

    return 0;
}

static border_operations_t empty_border_ops = {
    .paint = empty_border_paint
};

border_t* create_emptyborder( int left, int top, int right, int bottom ) {
    border_t* border;
    emptyborder_t* emptyborder;

    border = create_border( &empty_border_ops, sizeof(emptyborder_t) );

    if ( border == NULL ) {
        return NULL;
    }

    point_init( &border->size, left + right, top + bottom );
    point_init( &border->lefttop_offset, left, top );

    emptyborder = (emptyborder_t*)border_get_data(border);

    emptyborder->left = left;
    emptyborder->top = top;
    emptyborder->right = right;
    emptyborder->bottom = bottom;

    return border;
}
