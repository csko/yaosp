/* yaosp GUI library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <sys/param.h>
#include <yaosp/debug.h>

#include <ygui/panel.h>
#include <ygui/layout/flowlayout.h>

#include "../internal.h"

typedef struct flowlayout {
    int spacing;
} flowlayout_t;

static int flowlayout_do_layout( widget_t* widget ) {
    int i;
    int size;
    int left;
    int width;
    int height;
    rect_t bounds;
    int pref_w_sum;
    int w_inc_per_widget;

    layout_t* layout;
    flowlayout_t* flowlayout;

    layout = panel_get_layout(widget);
    flowlayout = (flowlayout_t*)( layout + 1 );

    widget_get_bounds( widget, &bounds );
    width = rect_width( &bounds );
    height = rect_height( &bounds );

    left = 0;
    pref_w_sum = 0;
    size = widget_get_child_count( widget );

    if ( size == 0 ) {
        return 0;
    }

    /* Calculate the sum of the child's preferred sizes. */

    for ( i = 0; i < size; i++ ) {
        widget_t* child;
        point_t pref_size;

        child = widget_get_child_at( widget, i );
        widget_get_preferred_size( child, &pref_size );

        pref_w_sum += pref_size.x;
    }

    pref_w_sum += ( size - 1 ) * flowlayout->spacing;

    if ( pref_w_sum < width ) {
        w_inc_per_widget = ( width - pref_w_sum ) / size;
    } else {
        w_inc_per_widget = 0;
    }

    /* Do the layout work ... */

    for ( i = 0; i < size; i++ ) {
        widget_t* child;
        point_t pref_size;
        point_t widget_position;
        point_t widget_size;

        child = widget_get_child_at( widget, i );
        widget_get_preferred_size( child, &pref_size );

        point_init( &widget_position, left, 0 );
        point_init( &widget_size, pref_size.x + w_inc_per_widget, height );

        widget_set_position_and_size( child, &widget_position, &widget_size );

        left += ( pref_size.x + w_inc_per_widget + flowlayout->spacing );
    }

    layout_dec_ref(layout);

    return 0;
}

static int flowlayout_get_preferred_size( widget_t* widget, point_t* size ) {
    int count;

    point_init( size, 0, 0 );

    count = widget_get_child_count( widget );

    if ( count > 0 ) {
        int i;

        for ( i = 0; i < count; i++ ) {
            point_t pref_size;
            widget_t* child = widget_get_child_at( widget, i );

            widget_get_preferred_size( child, &pref_size );

            size->y = MAX( size->y, pref_size.y );
            size->x += pref_size.x;
        }
    }

    return 0;
}

static layout_operations_t flowlayout_ops = {
    .do_layout = flowlayout_do_layout,
    .get_preferred_size = flowlayout_get_preferred_size
};

int flowlayout_set_spacing( layout_t* layout, int spacing ) {
    flowlayout_t* flowlayout;

    if ( ( layout == NULL ) ||
         ( layout_get_type(layout) != L_FLOW ) ) {
        return -EINVAL;
    }

    flowlayout = (flowlayout_t*)( layout + 1 );
    flowlayout->spacing = spacing;

    return 0;
}

layout_t* create_flowlayout( void ) {
    layout_t* layout;
    flowlayout_t* flowlayout;

    layout = create_layout( L_FLOW, &flowlayout_ops, sizeof(flowlayout_t) );

    if ( layout == NULL ) {
        return NULL;
    }

    flowlayout = (flowlayout_t*)( layout + 1 );
    flowlayout->spacing = 0;

    return layout;
}
