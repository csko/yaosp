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

#include <ygui/layout/borderlayout.h>

#include "../internal.h"

enum {
    W_PAGE_START,
    W_PAGE_END,
    W_LINE_START,
    W_LINE_END,
    W_CENTER,
    W_COUNT
};

static void borderlayout_do_page_start( widget_t* widget, point_t* panel_size, point_t* center_position, point_t* center_size ) {
    point_t widget_size;
    point_t widget_position;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        MIN( panel_size->x, maximum_size.x ),
        preferred_size.y
    );

    point_init(
        &widget_position,
        ( panel_size->x - widget_size.x ) / 2,
        0
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_position->y = widget_size.y;
    center_size->y -= widget_size.y;
}

static void borderlayout_do_page_end( widget_t* widget, point_t* panel_size, point_t* center_size ) {
    point_t widget_size;
    point_t widget_position;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        MIN( panel_size->x, maximum_size.x ),
        preferred_size.y
    );

    point_init(
        &widget_position,
        ( panel_size->x - widget_size.x ) / 2,
        panel_size->y - widget_size.y
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_size->y -= widget_size.y;
}

static void borderlayout_do_line_start( widget_t* widget, point_t* panel_size, point_t* center_position, point_t* center_size ) {
    point_t widget_position;
    point_t widget_size;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        preferred_size.x,
        MIN( center_size->y, maximum_size.y )
    );

    point_init(
        &widget_position,
        0,
        center_position->y + ( center_size->y - widget_size.y ) / 2
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_position->x += widget_size.x;
    center_size->x -= widget_size.x;
}

static void borderlayout_do_line_end( widget_t* widget, point_t* panel_size, point_t* center_position, point_t* center_size ) {
    point_t widget_position;
    point_t widget_size;
    point_t preferred_size;
    point_t maximum_size;

    widget_get_preferred_size( widget, &preferred_size );
    widget_get_maximum_size( widget, &maximum_size );

    point_init(
        &widget_size,
        preferred_size.x,
        MIN( center_size->y, maximum_size.y )
    );

    point_init(
        &widget_position,
        panel_size->x - widget_size.x,
        center_position->y + ( center_size->y - widget_size.y ) / 2
    );

    widget_set_position_and_size(
        widget,
        &widget_position,
        &widget_size
    );

    center_size->x -= widget_size.x;
}

static void borderlayout_do_center( widget_t* widget, point_t* panel_size, point_t* center_position, point_t* center_size ) {
    if ( center_size->y <= 0 ) {
        return;
    }

    widget_set_position_and_size(
        widget,
        center_position,
        center_size
    );
}

static int borderlayout_fill_widget_table( widget_t* widget, widget_t** table ) {
    int i;
    int count;

    count = array_get_size( &widget->children );

    for ( i = 0; i < count; i++ ) {
        widget_wrapper_t* wrapper;

        wrapper = ( widget_wrapper_t* )array_get_item( &widget->children, i );

        switch ( ( int )wrapper->data ) {
            case ( int )BRD_PAGE_START :
                table[ W_PAGE_START ] = wrapper->widget;
                break;

            case ( int )BRD_PAGE_END :
                table[ W_PAGE_END ] = wrapper->widget;
                break;

            case ( int )BRD_LINE_START :
                table[ W_LINE_START ] = wrapper->widget;
                break;

            case ( int )BRD_LINE_END :
                table[ W_LINE_END ] = wrapper->widget;
                break;

            case ( int )BRD_CENTER :
                table[ W_CENTER ] = wrapper->widget;
                break;
        }
    }

    return 0;
}

static int borderlayout_do_layout( widget_t* widget ) {
    rect_t tmp;
    point_t center_size;
    point_t center_position;
    point_t panel_size;
    widget_t* widget_table[ W_COUNT ] = { NULL, NULL, NULL, NULL, NULL };

    widget_get_bounds( widget, &tmp );
    panel_size.x = rect_width( &tmp );
    panel_size.y = rect_height( &tmp );

    borderlayout_fill_widget_table( widget, widget_table );

    point_init( &center_position, 0, 0 );
    point_copy( &center_size, &panel_size );

    if ( widget_table[ W_PAGE_START ] != NULL ) {
        borderlayout_do_page_start(
            widget_table[ W_PAGE_START ],
            &panel_size,
            &center_position,
            &center_size
        );
    }

    if ( widget_table[ W_PAGE_END ] != NULL ) {
        borderlayout_do_page_end(
            widget_table[ W_PAGE_END ],
            &panel_size,
            &center_size
        );
    }

    if ( widget_table[ W_LINE_START ] != NULL ) {
        borderlayout_do_line_start(
            widget_table[ W_LINE_START ],
            &panel_size,
            &center_position,
            &center_size
        );
    }

    if ( widget_table[ W_LINE_END ] != NULL ) {
        borderlayout_do_line_end(
            widget_table[ W_LINE_END ],
            &panel_size,
            &center_position,
            &center_size
        );
    }

    if ( widget_table[ W_CENTER ] != NULL ) {
        borderlayout_do_center(
            widget_table[ W_CENTER ],
            &panel_size,
            &center_position,
            &center_size
        );
    }

    return 0;
}

static int borderlayout_get_preferred_size( widget_t* widget, point_t* size ) {
    int i;
    point_t pref_size[ W_COUNT ];
    widget_t* widget_table[ W_COUNT ] = { NULL, NULL, NULL, NULL, NULL };

    borderlayout_fill_widget_table( widget, widget_table );

    for ( i = 0; i < W_COUNT; i++ ) {
        if ( widget_table[ i ] == NULL ) {
            point_init( &pref_size[ i ], 0, 0 );
        } else {
            widget_get_preferred_size( widget_table[ i ], &pref_size[ i ] );
        }
    }

    size->x = pref_size[ W_LINE_START ].x +
              pref_size[ W_CENTER ].x +
              pref_size[ W_LINE_END ].x;

    size->y = pref_size[ W_PAGE_START ].y +
              pref_size[ W_CENTER ].y +
              pref_size[ W_PAGE_END ].y;
    size->y = MAX( size->y, pref_size[ W_PAGE_START ].y +
                            pref_size[ W_LINE_START ].y +
                            pref_size[ W_PAGE_END ].y );
    size->y = MAX( size->y, pref_size[ W_PAGE_START ].y +
                            pref_size[ W_LINE_END ].y +
                            pref_size[ W_PAGE_END ].y );

    return 0;
}

static layout_operations_t borderlayout_ops = {
    .do_layout = borderlayout_do_layout,
    .get_preferred_size = borderlayout_get_preferred_size
};

layout_t* create_border_layout( void ) {
    layout_t* layout;

    layout = create_layout( &borderlayout_ops );

    if ( layout == NULL ) {
        return NULL;
    }

    return layout;
}
