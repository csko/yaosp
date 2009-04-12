/* GUI server
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

#include <assert.h>
#include <errno.h>

#include <windowdecorator.h>
#include <graphicsdriver.h>
#include <mouse.h>
#include <windowmanager.h>

#define BORDER_SIZE 5

static color_t border_colors[] = {
    { 0x4E, 0x7A, 0xA0, 0xFF },
    { 0x83, 0xA7, 0xC6, 0xFF },
    { 0x74, 0x9A, 0xBB, 0xFF },
    { 0x5D, 0x86, 0xA9, 0xFF },
    { 0x5C, 0x5C, 0x5B, 0xFF }
};

static color_t top_border_colors[] = {
    { 0x4E, 0x7A, 0xA0, 0xFF },
    { 0x83, 0xA7, 0xC6, 0xFF },
    { 0x74, 0x9A, 0xBB, 0xFF },
    { 0x5D, 0x86, 0xA9, 0xFF },
    { 0x5D, 0x86, 0xA9, 0xFF },
    { 0x5D, 0x86, 0xA8, 0xFF },
    { 0x5D, 0x85, 0xA8, 0xFF },
    { 0x5C, 0x84, 0xA7, 0xFF },
    { 0x5B, 0x83, 0xA6, 0xFF },
    { 0x5B, 0x83, 0xA5, 0xFF },
    { 0x5A, 0x82, 0xA4, 0xFF },
    { 0x5A, 0x81, 0xA2, 0xFF },
    { 0x5A, 0x80, 0xA1, 0xFF },
    { 0x58, 0x7F, 0xA0, 0xFF },
    { 0x58, 0x7E, 0x9F, 0xFF },
    { 0x57, 0x7E, 0x9E, 0xFF },
    { 0x56, 0x7C, 0x9C, 0xFF },
    { 0x56, 0x7B, 0x9B, 0xFF },
    { 0x56, 0x7B, 0x9A, 0xFF },
    { 0x55, 0x7A, 0x99, 0xFF },
    { 0x54, 0x79, 0x98, 0xFF },
    { 0x54, 0x78, 0x97, 0xFF },
    { 0x54, 0x77, 0x96, 0xFF },
    { 0x54, 0x77, 0x96, 0xFF },
    { 0x53, 0x77, 0x95, 0xFF },
    { 0x5C, 0x5C, 0x5B, 0xFF }
};

typedef struct decorator_data {
    rect_t header;
    rect_t left_border;
    rect_t right_border;
    rect_t bottom_border;
} decorator_data_t;

static int decorator_initialize( window_t* window ) {
    decorator_data_t* data;

    data = ( decorator_data_t* )malloc( sizeof( decorator_data_t ) );

    if ( data == NULL ) {
        return -ENOMEM;
    }

    window->decorator_data = ( void* )data;

    return 0;
}

static int decorator_destroy( window_t* window ) {
    assert( window->decorator_data != NULL );

    free( window->decorator_data );
    window->decorator_data = NULL;

    return 0;
}

static int decorator_calculate_regions( window_t* window ) {
    rect_t* screen_rect;
    decorator_data_t* data;

    assert( window->decorator_data != NULL );

    data = window->decorator_data;

    screen_rect = &window->screen_rect;

    /* Header rect */

    rect_init(
        &data->header,
        screen_rect->left,
        screen_rect->top,
        screen_rect->right,
        screen_rect->top + 25
    );

    /* Left border */

    rect_init(
        &data->left_border,
        screen_rect->left,
        screen_rect->top,
        screen_rect->left + 4,
        screen_rect->bottom
    );

    /* Right border */

    rect_init(
        &data->right_border,
        screen_rect->right - 4,
        screen_rect->top,
        screen_rect->right,
        screen_rect->bottom
    );

    /* Bottom border */

    rect_init(
        &data->bottom_border,
        screen_rect->left,
        screen_rect->bottom - 4,
        screen_rect->right,
        screen_rect->bottom
    );

    return 0;
}

static int decorator_update_border( window_t* window ) {
    int i;
    int width;
    int height;
    rect_t rect;
    color_t* color;
    bitmap_t* bitmap;

    bitmap = window->bitmap;

    rect_bounds( &window->screen_rect, &width, &height );

    color = &border_colors[ 0 ];

    for ( i = 0; i < 5; i++, color++ ) {
        /* Left | */

        rect_init( &rect, i, i, i, height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );

        /* Right | */

        rect_init( &rect, width - ( i + 1 ), i, width - ( i + 1 ), height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );

        /* Bottom - */

        rect_init( &rect, i, height - ( i + 1 ), width - ( i + 1 ), height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );
    }

    /* Top - */

    color = &top_border_colors[ 0 ];

    for ( i = 0; i < 4; i++, color++ ) {
        rect_init( &rect, i, i, width - ( i + 1 ), i );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );
    }

    for ( i = 4; i < 26; i++, color++ ) {
        rect_init( &rect, 4, i, width - 5, i );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );
    }

    return 0;
}

static int decorator_border_has_position( window_t* window, point_t* position ) {
    decorator_data_t* data;

    assert( window->decorator_data != NULL );

    data = ( decorator_data_t* )window->decorator_data;

    if ( ( rect_has_point( &data->header, position ) ) ||
         ( rect_has_point( &data->left_border, position ) ) ||
         ( rect_has_point( &data->right_border, position ) ) ||
         ( rect_has_point( &data->bottom_border, position ) ) ) {
        return 1;
    }

    return 0;
}

static int decorator_mouse_entered( window_t* window, point_t* position ) {
    return 0;
}

static int decorator_mouse_exited( window_t* window ) {
    return 0;
}

static int decorator_mouse_moved( window_t* window, point_t* position ) {
    return 0;
}

static int decorator_mouse_pressed( window_t* window, int button ) {
    decorator_data_t* data;
    point_t mouse_position;

    assert( window->decorator_data != NULL );

    data = ( decorator_data_t* )window->decorator_data;

    mouse_get_position( &mouse_position );

    if ( rect_has_point( &data->header, &mouse_position ) ) {
        wm_set_moving_window( window );
    }

    return 0;
}

static int decorator_mouse_released( window_t* window, int button ) {
    if ( window->is_moving ) {
        wm_set_moving_window( NULL );
    }

    return 0;
}

window_decorator_t default_decorator = {
    .border_size = {
        .x = 10,
        .y = 31
    },
    .lefttop_offset = {
        .x = 5,
        .y = 26
    },
    .initialize = decorator_initialize,
    .destroy = decorator_destroy,
    .calculate_regions = decorator_calculate_regions,
    .update_border = decorator_update_border,
    .border_has_position = decorator_border_has_position,
    .mouse_entered = decorator_mouse_entered,
    .mouse_exited = decorator_mouse_exited,
    .mouse_moved = decorator_mouse_moved,
    .mouse_pressed = decorator_mouse_pressed,
    .mouse_released = decorator_mouse_released
};
