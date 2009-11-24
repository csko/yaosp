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
#include <sys/param.h>
#include <yaosp/debug.h>

#include <ygui/layout/flowlayout.h>

#include "../internal.h"

static int flowlayout_do_layout( widget_t* widget ) {
    int i;
    int size;
    int left;
    int height;
    rect_t bounds;

    widget_get_bounds( widget, &bounds );
    height = rect_height( &bounds );

    left = 0;
    size = widget_get_child_count( widget );

    for ( i = 0; i < size; i++ ) {
        widget_t* child;
        point_t pref_size;
        point_t widget_position;
        point_t widget_size;

        child = widget_get_child_at( widget, i );
        widget_get_preferred_size( child, &pref_size );

        point_init( &widget_position, left, 0 );
        point_init( &widget_size, pref_size.x, height );

        widget_set_position_and_size( child, &widget_position, &widget_size );

        left += pref_size.x;
    }

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

layout_t* create_flow_layout( void ) {
    layout_t* layout;

    layout = create_layout( &flowlayout_ops );

    if ( layout == NULL ) {
        goto error1;
    }

    return layout;

 error1:
    return NULL;
}
