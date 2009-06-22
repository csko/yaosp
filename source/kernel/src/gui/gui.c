/* Graphical User Interface
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

#include <config.h>

#ifdef ENABLE_GUI

#include <errno.h>

#include <gui/gui.h>
#include <gui/graphicsdriver.h>

static int setup_video_mode( void ) {
    int error;
    int found;
    uint32_t i;
    uint32_t count;
    screen_mode_t screen_mode;
    graphics_driver_t* driver;

    driver = get_graphics_driver();

    if ( driver == NULL ) {
        return -EINVAL;
    }

    found = 0;
    count = driver->get_screen_mode_count();

    for ( i = 0; i < count; i++ ) {
        error = driver->get_screen_mode_info( i, &screen_mode );

        if ( error < 0 ) {
            continue;
        }

        if ( ( screen_mode.width == 640 ) &&
             ( screen_mode.height == 480 ) &&
             ( screen_mode.color_space == CS_RGB32 ) ) {
            found = 1;

            break;
        }
    }

    if ( !found ) {
        return -ENOENT;
    }

    error = driver->set_screen_mode( &screen_mode );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int gui_start( void ) {
    int error;

    error = select_graphics_driver();

    if ( error < 0 ) {
        return error;
    }

    error = setup_video_mode();

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

#endif /* ENABLE_GUI */
