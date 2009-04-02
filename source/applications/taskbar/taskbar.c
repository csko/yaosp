/* Taskbar application
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
#include <yaosp/debug.h>

#include <ygui/application.h>
#include <ygui/window.h>

int main( int argc, char** argv ) {
    int error;
    window_t* win;

    error = create_application();

    if ( error < 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return error;
    }

    point_t point = { .x = 50, .y = 50 };
    point_t size = { .x = 100, .y = 100 };

    win = create_window( "Taskbar", &point, &size, 0 );
    show_window( win );

    run_application();

    return 0;
}
