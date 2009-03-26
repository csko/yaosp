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

#include "../driver/video/vesa/vesa.h"

static bitmap_t* screen_bitmap;
static screen_mode_t active_screen_mode;
static graphics_driver_t* graphics_driver = NULL;

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

    color_t tmp_color = { 0x00, 0x00, 0x00, 0x00 };
    rect_t tmp_rect = { 0, 0, 639, 479 };

    graphics_driver->fill_rect( screen_bitmap, &tmp_rect, &tmp_color );

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

    return 0;
}
