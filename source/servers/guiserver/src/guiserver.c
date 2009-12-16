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
#include <yaosp/ipc.h>

#include <ygui/protocol.h>

#include <graphicsdriver.h>
#include <bitmap.h>
#include <fontmanager.h>
#include <input.h>
#include <mouse.h>
#include <application.h>
#include <windowmanager.h>
#include <splash.h>

#include "../driver/video/vesa/vesa.h"

rect_t screen_rect;
bitmap_t* screen_bitmap;
screen_mode_t active_screen_mode;
graphics_driver_t* graphics_driver;

int init_default_decorator( void );

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

    rect_init(
        &screen_rect,
        0, 0,
        active_screen_mode.width - 1, active_screen_mode.height - 1
    );

    return 0;
}

#define MAX_GUISERVER_BUFSIZE 512

static int guiserver_mainloop( void ) {
    int error;
    ipc_port_id guiserver_port;

    uint32_t code;
    void* buffer;

    buffer = malloc( MAX_GUISERVER_BUFSIZE );

    if ( buffer == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    guiserver_port = create_ipc_port();

    if ( guiserver_port < 0 ) {
        error = guiserver_port;
        goto error2;
    }

    error = register_named_ipc_port( "guiserver", guiserver_port );

    if ( error < 0 ) {
        goto error3;
    }

    splash_inc_progress();

    while ( 1 ) {
        error = recv_ipc_message( guiserver_port, &code, buffer, MAX_GUISERVER_BUFSIZE, INFINITE_TIMEOUT );

        if ( error < 0 ) {
            continue;
        }

        switch ( code ) {
            case MSG_APPLICATION_CREATE :
                handle_create_application( ( msg_create_app_t* )buffer );
                break;

            case MSG_TASKBAR_STARTED :
                wm_enable();
                input_system_start();
                show_mouse_pointer();
                break;

            default :
                dbprintf( "guiserver_mainloop(): Unknown message code: %x\n", code );
                break;
        }
    }

    free( buffer );

    return 0;

error3:
    /* TOOD: delete the guiserver port! */

error2:
    free( buffer );

error1:
    return error;
}

int main( int argc, char** argv ) {
    int error;

    if ( init_bitmap() != 0 ) {
        printf( "Failed to initialize bitmaps\n" );
        return EXIT_FAILURE;
    }

    if ( ( choose_graphics_driver() != 0 ) ||
         ( choose_graphics_mode() != 0 ) ) {
        printf( "Failed to select proper graphics driver and/or mode!\n" );
        return EXIT_FAILURE;
    }

    printf( "Using graphics driver: %s\n", graphics_driver->name );
    printf(
        "Using screen mode: %dx%dx%d\n",
        active_screen_mode.width, active_screen_mode.height,
        colorspace_to_bpp( active_screen_mode.color_space ) * 4
    );

    if ( setup_graphics_mode() != 0 ) {
        dbprintf( "Failed to setup graphics mode!\n" );
        return EXIT_FAILURE;
    }

    init_splash();

    splash_count_total = 5 /* + font_count */;

    if (  init_font_manager() != 0 ) {
        dbprintf( "Failed to initialize font manager\n" );
        return error;
    }

    font_manager_load_fonts();

    if ( init_default_decorator() != 0 ) {
        dbprintf( "Failed to initialize default window decorator\n" );
        return EXIT_FAILURE;
    }

    splash_inc_progress();

    if ( init_mouse_manager() != 0 ) {
        printf( "Failed to initialize mouse manager\n" );
        return error;
    }

    splash_inc_progress();

    if ( init_windowmanager() != 0 ) {
        printf( "Failed to initialize window manager\n" );
        return error;
    }

    splash_inc_progress();

    if ( init_input_system() != 0 ) {
        dbprintf( "Failed to initialize input system!\n" );
        return error;
    }

    splash_inc_progress();

    if (  guiserver_mainloop() != 0 ) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
