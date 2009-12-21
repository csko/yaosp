/* GUI server - default window decorator
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
#include <sys/types.h>

#include <windowdecorator.h>
#include <graphicsdriver.h>
#include <mouse.h>
#include <windowmanager.h>
#include <bitmap.h>

#define BORDER_LEFT    3
#define BORDER_TOP    21
#define BORDER_RIGHT   3
#define BORDER_BOTTOM  3

static color_t top_border_colors[ BORDER_TOP ] = {
    { 0x6D, 0x6D, 0x6D, 0xFF },
    { 0x5D, 0x5D, 0x5D, 0xFF },
    { 0x50, 0x50, 0x50, 0xFF },
    { 0x48, 0x48, 0x48, 0xFF },
    { 0x45, 0x45, 0x45, 0xFF },
    { 0x45, 0x45, 0x45, 0xFF },
    { 0x46, 0x46, 0x46, 0xFF },
    { 0x48, 0x48, 0x48, 0xFF },
    { 0x4A, 0x4A, 0x4A, 0xFF },
    { 0x4B, 0x4B, 0x4B, 0xFF },
    { 0x4D, 0x4D, 0x4D, 0xFF },
    { 0x4F, 0x4F, 0x4F, 0xFF },
    { 0x51, 0x51, 0x51, 0xFF },
    { 0x52, 0x52, 0x52, 0xFF },
    { 0x53, 0x53, 0x53, 0xFF },
    { 0x55, 0x55, 0x55, 0xFF },
    { 0x54, 0x54, 0x54, 0xFF },
    { 0x52, 0x52, 0x52, 0xFF },
    { 0x4B, 0x4B, 0x4B, 0xFF },
    { 0x41, 0x41, 0x41, 0xFF },
    { 0x33, 0x33, 0x33, 0xFF }
};

static uint8_t close_focused[ 21 * 21 * 4 ] = {
#include "images/close_focus.c"
};

typedef struct decorator_data {
    rect_t header;
    rect_t left_border;
    rect_t right_border;
    rect_t bottom_border;

    rect_t close_button;
    rect_t drag_region;
} decorator_data_t;

static font_node_t* title_font;
static bitmap_t* bmp_close_focused;

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
    if ( window->decorator_data == NULL ) {
        return 0;
    }

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
        screen_rect->top + 20
    );

    /* Left border */

    rect_init(
        &data->left_border,
        screen_rect->left,
        screen_rect->top,
        screen_rect->left + BORDER_LEFT - 1,
        screen_rect->bottom
    );

    /* Right border */

    rect_init(
        &data->right_border,
        screen_rect->right - BORDER_RIGHT + 1,
        screen_rect->top,
        screen_rect->right,
        screen_rect->bottom
    );

    /* Bottom border */

    rect_init(
        &data->bottom_border,
        screen_rect->left,
        screen_rect->bottom - BORDER_BOTTOM + 1,
        screen_rect->right,
        screen_rect->bottom
    );

    /* Close button */

    rect_init(
        &data->close_button,
        screen_rect->right - 21 + 1,
        screen_rect->top,
        screen_rect->right,
        screen_rect->top + 21 - 1
    );

    /* Drag region */

    rect_init(
        &data->drag_region,
        screen_rect->right - 15 + 1,
        screen_rect->bottom - BORDER_BOTTOM + 1,
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
    point_t point;
    color_t* color;
    color_t tmp_color;
    bitmap_t* bitmap;

    bitmap = window->bitmap;
    rect_bounds_xy( &window->screen_rect, &width, &height );

    color = &top_border_colors[ BORDER_TOP - 1 ];

    /* Left | */

    for ( i = 0; i < BORDER_LEFT; i++ ) {
        rect_init( &rect, i, i, i, height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );
    }

    /* Right | */

    for ( i = 0; i < BORDER_RIGHT; i++ ) {
        rect_init( &rect, width - ( i + 1 ), i, width - ( i + 1 ), height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );
    }

    /* Bottom - */

    for ( i = 0; i < BORDER_BOTTOM; i++ ) {
        rect_init( &rect, i, height - ( i + 1 ), width - ( i + 1 ), height - ( i + 1 ) );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );
    }

    /* Top - */

    color = &top_border_colors[ 0 ];

    for ( i = 0; i < BORDER_TOP; i++, color++ ) {
        rect_init( &rect, 0, i, width - 1, i );
        graphics_driver->fill_rect( bitmap, &rect, color, DM_COPY );
    }

    /* Close button */

    point_init( &point, width - 22, 0 );
    rect_init( &rect, 0, 0, 20, 20 );

    graphics_driver->blit_bitmap(
        bitmap, &point,
        bmp_close_focused,
        &rect, DM_COPY
    );

    /* Window title */

    point_init(
        &point,
        ( bitmap->width - font_node_get_string_width( title_font, window->title, strlen( window->title ) ) ) / 2,
        ( BORDER_TOP - ( title_font->ascender - title_font->descender ) ) / 2 + title_font->ascender
    );
    rect_init(
        &rect,
        20, 0,
        bitmap->width - ( 22 + 1 ),
        BORDER_TOP - 1
    );
    color_init(
        &tmp_color,
        255, 255, 255, 0
    );

    graphics_driver->draw_text(
        bitmap, &point,
        &rect, title_font,
        &tmp_color, window->title,
        -1
    );

    /* Window icon */

    if ( window->icon != NULL ) {
        point_init(
            &point,
            3, ( BORDER_TOP - window->icon->height ) / 2
        );
        rect_init(
            &rect,
            0, 0,
            window->icon->width - 1, window->icon->height - 1
        );

        graphics_driver->blit_bitmap(
            bitmap,
            &point,
            window->icon,
            &rect,
            DM_BLEND
        );
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

    if ( rect_has_point( &data->close_button, &mouse_position ) ) {
        window_close_request( window );
    } else if ( rect_has_point( &data->header, &mouse_position ) ) {
        wm_set_moving_window( window );
    } else if ( ( ( window->flags & WINDOW_FIXED_SIZE ) == 0 ) &&
                ( rect_has_point( &data->drag_region, &mouse_position ) ) ) {
        wm_set_resizing_window( window );
    }

    return 0;
}

static int decorator_mouse_released( window_t* window, int button ) {
    if ( window->is_moving ) {
        wm_set_moving_window( NULL );
    } else if ( window->is_resizing ) {
        wm_set_resizing_window( NULL );
    }

    return 0;
}

int init_default_decorator( void ) {
    font_properties_t properties;

    bmp_close_focused = create_bitmap_from_buffer(
        21, 21, CS_RGB32,
        close_focused
    );

    if ( bmp_close_focused == NULL ) {
        goto error1;
    }

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    title_font = font_manager_get( "DejaVu Sans", "Bold", &properties );

    if ( title_font == NULL ) {
        goto error2;
    }

    return 0;

 error2:
    /* TODO: free close button bitmap */

 error1:
    return -ENOMEM;
}

window_decorator_t default_decorator = {
    .border_size = {
        .x = BORDER_LEFT + BORDER_RIGHT,
        .y = BORDER_TOP + BORDER_BOTTOM
    },
    .lefttop_offset = {
        .x = BORDER_LEFT,
        .y = BORDER_TOP
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
