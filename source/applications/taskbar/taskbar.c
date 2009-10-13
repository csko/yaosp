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
#include <stdlib.h>
#include <unistd.h>
#include <yaosp/debug.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/textfield.h>
#include <ygui/button.h>
#include <ygui/desktop.h>
#include <ygui/image.h>
#include <ygui/menu.h>
#include <ygui/menuitem.h>
#include <ygui/layout/borderlayout.h>

static int event_open_terminal( widget_t* widget, void* data ) {
    if ( fork() == 0 ) {
        char* argv[] = { "terminal", NULL };
        execv( "/application/terminal", argv );
        _exit( 1 );
    }

    return 0;
}

static int event_open_taskbar( widget_t* widget, void* data ) {
    menu_t* menu = create_menu();
    widget_t* item = create_menuitem_with_label_and_image(
        "Terminal",
        bitmap_load_from_file( "/application/taskbar/images/terminal.png" )
    );

    menu_add_item( menu, item );
    menu_popup_at_xy( menu, 0, 40 );

    widget_connect_event_handler( item, "clicked", event_open_terminal, NULL );

    return 0;
}

int main( int argc, char** argv ) {
    int error;
    window_t* win;

    error = create_application();

    if ( error < 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return error;
    }

    point_t point = { .x = 0, .y = 0 };
    point_t size;

    desktop_get_size( &size );

    size.y = 40;

    /* Create a window */

    win = create_window( "Taskbar", &point, &size, WINDOW_NO_BORDER );

    widget_t* container = window_get_container( win );

    /* Set the layout of the window */

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    widget_t* image = create_image( bitmap_load_from_file( "/application/taskbar/images/start.png" ) );
    widget_add( container, image, BRD_LINE_START );
    widget_dec_ref( image );

    widget_connect_event_handler( image, "mouse-down", event_open_taskbar, NULL );

    /* Show the window */

    window_show( win );

    run_application();

    return 0;
}
