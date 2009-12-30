/* Desktop application
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
#include <yaosp/ipc.h>
#include <yaosp/yaosp.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/image.h>
#include <ygui/desktop.h>
#include <ygui/protocol.h>
#include <ygui/layout/borderlayout.h>

#include <yutil/process.h>

window_t* window;

extern ipc_port_id guiserver_port;

static int send_guiserver_notification( void ) {
    send_ipc_message( guiserver_port, MSG_DESKTOP_STARTED, NULL, 0 );
    return 0;
}

int main( int argc, char** argv ) {
    point_t pos;
    point_t size;

    if ( process_count_of( "desktop" ) != 1 ) {
        fprintf( stderr, "Taskbar is already running!\n" );
        return EXIT_FAILURE;
    }

    if ( application_init() != 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return EXIT_FAILURE;
    }

    point_init( &pos, 0, 0 );
    desktop_get_size( &size );

    /* Create a window */

    window = create_window( "Desktop", &pos, &size, W_ORDER_ALWAYS_ON_BOTTOM, WINDOW_NO_BORDER );

    widget_t* container = window_get_container( window );

    /* Set the layout of the window */

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    bitmap_t* bitmap = bitmap_load_from_file( "/application/desktop/images/background.png" );

    widget_t* image = create_image( bitmap );
    widget_add( container, image, BRD_CENTER );
    widget_dec_ref( image );

    bitmap_dec_ref( bitmap );

    window_show( window );

    send_guiserver_notification();

    /* Start the mainloop of the application ... */

    application_run();

    return EXIT_SUCCESS;
}
