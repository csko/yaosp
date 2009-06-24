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

#include <ygui/scrollpanel.h>

typedef struct scrollpanel {
    /* Vertical scrollbar */

    int v_scroll_visible;
    scrollbar_policy_t v_scroll_policy;

    /* Horizontal scrollbar */

    int h_scroll_visible;
    scrollbar_policy_t h_scroll_policy;
} scrollpanel_t;

static void paint_v_scrollbar( widget_t* widget, gc_t* gc ) {
    rect_t tmp;
    rect_t bounds;

    static color_t fg_color = { 0, 0, 0, 0xFF };
    static color_t bg_color = { 216, 216, 216, 0xFF };

    widget_get_bounds( widget, &bounds );

    /* Border */

    rect_init(
        &tmp,
        bounds.right - 15 + 1,
        bounds.top,
        bounds.right,
        bounds.bottom
    );

    gc_set_pen_color( gc, &fg_color );
    gc_draw_rect( gc, &tmp );

    /* Up button */

    rect_init(
        &tmp,
        bounds.right - 15 + 2,
        bounds.top + 15 - 1,
        bounds.right - 1,
        bounds.top + 15 - 1
    );

    gc_fill_rect( gc, &tmp );

    /* Down button */

    rect_init(
        &tmp,
        bounds.right - 15 + 2,
        bounds.bottom - 15 + 1,
        bounds.right - 1,
        bounds.bottom - 15 + 1
    );

    gc_fill_rect( gc, &tmp );
}

static void paint_h_scrollbar( widget_t* widget, gc_t* gc ) {
}

static int scrollpanel_paint( widget_t* widget, gc_t* gc ) {
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )widget_get_data( widget );

    if ( scrollpanel->v_scroll_visible ) {
        paint_v_scrollbar( widget, gc );
    }

    if ( scrollpanel->h_scroll_visible ) {
        paint_h_scrollbar( widget, gc );
    }

    return 0;
}

static int scrollpanel_get_preferred_size( widget_t* widget, point_t* size ) {
    if ( widget_get_child_count( widget ) == 0 ) {
        point_init(
            size,
            0,
            0
        );

        return 0;
    }

    widget_get_preferred_size( widget_get_child( widget, 0 ), size );

    return 0;
}

static widget_operations_t scrollpanel_ops = {
    .paint = scrollpanel_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = scrollpanel_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL
};

widget_t* create_scroll_panel( scrollbar_policy_t v_scroll_policy, scrollbar_policy_t h_scroll_policy ) {
    widget_t* widget;
    scrollpanel_t* scrollpanel;

    scrollpanel = ( scrollpanel_t* )malloc( sizeof( scrollpanel_t ) );

    if ( scrollpanel == NULL ) {
        goto error1;
    }

    widget = create_widget( W_SCROLLPANEL, &scrollpanel_ops, scrollpanel );

    if ( widget == NULL ) {
        goto error2;
    }

    scrollpanel->v_scroll_visible = 1;
    scrollpanel->h_scroll_visible = 1;
    scrollpanel->v_scroll_policy = v_scroll_policy;
    scrollpanel->h_scroll_policy = h_scroll_policy;

    return widget;

 error2:
    free( scrollpanel );

 error1:
    return NULL;
}
