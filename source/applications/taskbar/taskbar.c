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
#include <ygui/panel.h>
#include <ygui/label.h>
#include <ygui/layout/borderlayout.h>

int main( int argc, char** argv ) {
    int error;
    window_t* win;

    error = create_application();

    if ( error < 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return error;
    }

    point_t point = { .x = 50, .y = 50 };
    point_t size = { .x = 300, .y = 300 };

    /* Create a window */

    win = create_window( "Taskbar", &point, &size, 0 );

    widget_t* container = window_get_container( win );

    /* Set the layout of the window */

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    /* Create a test label */

    point_t p = { 25, 25 };
    point_t s = { 100, 25 };

    widget_t* label = create_label( "Hello World" );
    widget_add( container, label );
    widget_set_position( label, &p );
    widget_set_size( label, &s );
    widget_dec_ref( label );

    /* Show the window */

    show_window( win );

    run_application();

    return 0;
}
