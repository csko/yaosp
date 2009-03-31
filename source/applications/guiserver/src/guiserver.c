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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <yaosp/debug.h>

#include <graphicsdriver.h>
#include <bitmap.h>
#include <fontmanager.h>
#include <input.h>
#include <mouse.h>

#include "../driver/video/vesa/vesa.h"

rect_t screen_rect;
bitmap_t* screen_bitmap;
screen_mode_t active_screen_mode;
graphics_driver_t* graphics_driver;

static graphics_driver_t* graphics_drivers[] = {
    &vesa_graphics_driver,
    NULL
};

static int choose_graphics_driver( void ) {
    uint32_t i;
    graphics_driver_t* driver;

    for ( i = 0; graphics_drivers[ i ] != NULL; i++ ) {
        driver = graphics_drivers[ i ];

        if ( driver->detect() == 0 ) {
            graphics_driver = driver;
            return 0;
        }
    }

    return -1;
}

static int choose_graphics_mode( void ) {
    int error;
    uint32_t i;
    uint32_t count;
    screen_mode_t screen_mode;

    count = graphics_driver->get_screen_mode_count();

    for ( i = 0; i < count; i++ ) {
        error = graphics_driver->get_screen_mode_info( i, &screen_mode );

        if ( error < 0 ) {
            continue;
        }

        if ( ( screen_mode.width == 640 ) &&
             ( screen_mode.height == 480 ) &&
             ( screen_mode.color_space == CS_RGB32 ) ) {
            memcpy( &active_screen_mode, &screen_mode, sizeof( screen_mode_t ) );

            return 0;
        }
    }

    return -1;
}

static int setup_graphics_mode( void ) {
    int error;
    void* fb_address;

    error = graphics_driver->set_screen_mode( &active_screen_mode );

    if ( error < 0 ) {
        return error;
    }

    error = graphics_driver->get_framebuffer_info( &fb_address );

    if ( error < 0 ) {
        return error;
    }

    screen_bitmap = create_bitmap_from_buffer(
        active_screen_mode.width,
        active_screen_mode.height,
        active_screen_mode.color_space,
        fb_address
    );

    if ( screen_bitmap == NULL ) {
        return -ENOMEM;
    }

    rect_init( &screen_rect, 0, 0, active_screen_mode.width - 1, active_screen_mode.height - 1 );

    color_t tmp_color = { 0x11, 0x22, 0x33, 0x00 };

    graphics_driver->fill_rect( screen_bitmap, &screen_rect, &tmp_color );

    font_properties_t properties = { .point_size = 18 * 64, .flags = FONT_SMOOTHED };
    font_node_t* font_node = get_font( "DejaVu Sans", "Book", &properties );

    if ( font_node == NULL ) {
        dbprintf( "Font node not found!\n" );
    } else {
        point_t p = { .x = 100, .y = 100 };
        tmp_color.red = 0xff; tmp_color.green = 0xff; tmp_color.blue = 0xff;

        graphics_driver->draw_text(
            screen_bitmap,
            &p,
            &screen_rect,
            font_node,
            &tmp_color,
            "Hello World",
            -1
        );
    }

    return 0;
}

int main( int argc, char** argv ) {
    int error;

    error = init_bitmap();

    if ( error < 0 ) {
        printf( "Failed to initialize bitmaps\n" );
        return error;
    }

    error = init_font_manager();

    if ( error < 0 ) {
        printf( "Failed to initialize font manager\n" );
        return error;
    }

    error = init_mouse_manager();

    if ( error < 0 ) {
        printf( "Failed to initialize mouse manager\n" );
        return error;
    }

    error = choose_graphics_driver();

    if ( error < 0 ) {
        printf( "Failed to select proper graphics driver!\n" );
        return error;
    }

    printf( "Using graphics driver: %s\n", graphics_driver->name );

    error = choose_graphics_mode();

    if ( error < 0 ) {
        printf( "Failed to select proper graphics mode!\n" );
        return error;
    }

    printf(
        "Using screen mode: %dx%dx%d\n",
        active_screen_mode.width,
        active_screen_mode.height,
        colorspace_to_bpp( active_screen_mode.color_space ) * 4
    );

    error = setup_graphics_mode();

    if ( error < 0 ) {
        dbprintf( "Failed to setup graphics mode!\n" );
        return error;
    }

    show_mouse_pointer();

    error = init_input_system();

    if ( error < 0 ) {
        dbprintf( "Failed to initialize input system!\n" );
        return error;
    }

    return 0;
}
