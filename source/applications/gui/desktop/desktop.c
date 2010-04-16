/* Desktop application
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <yaosp/config.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/image.h>
#include <ygui/desktop.h>
#include <ygui/protocol.h>
#include <ygui/layout/borderlayout.h>
#include <yutil/process.h>

#include "wallpaper.h"

window_t* window;
widget_t* image_wallpaper;

static int desktop_handle_resolution_change( msg_scr_res_changed_t* msg ) {
    point_t size;

    point_init( &size, msg->mode_info.width, msg->mode_info.height );
    window_resize( window, &size );

    /* Resize the background image as well. */

    wallpaper_resize(msg->mode_info.width, msg->mode_info.height);

    return 0;
}

static int desktop_change_wallpaper( char* path ) {
    wallpaper_set(path);

    return 0;
}

static int desktop_msg_handler( uint32_t code, void* data ) {
    switch ( code ) {
        case MSG_SCREEN_RESOLUTION_CHANGED :
            desktop_handle_resolution_change( ( msg_scr_res_changed_t* )data );
            break;

        case MSG_DESKTOP_CHANGE_WALLPAPER :
            desktop_change_wallpaper( ( char* )data );
            break;

        default :
            return -1;
    }

    return 0;
}

static int send_guiserver_notification( void ) {
    send_ipc_message( application_get_guiserver_port(), MSG_DESKTOP_STARTED, NULL, 0 );
    return 0;
}

int main( int argc, char** argv ) {
    point_t pos;
    point_t size;
    char* img_file;

    if ( process_count_of( "desktop" ) != 1 ) {
        fprintf( stderr, "Taskbar is already running!\n" );
        return EXIT_FAILURE;
    }

    ycfg_init();
    wallpaper_init();

    if ( application_init( APP_NOTIFY_RESOLUTION_CHANGE ) != 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return EXIT_FAILURE;
    }

    point_init( &pos, 0, 0 );
    desktop_get_size( &size );
    point_copy( &wallpaper_size, &size );

    register_named_ipc_port( "desktop", application_get_client_port() );

    /* Create a window */

    window = create_window( "Desktop", &pos, &size, W_ORDER_ALWAYS_ON_BOTTOM, WINDOW_NO_BORDER );

    widget_t* container = window_get_container( window );

    static color_t black = { 0, 0, 0, 255 };
    panel_set_background_color( container, &black );

    /* Set the layout of the window */

    layout_t* layout = create_borderlayout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    /* Create and add the background image widget to the window. */

    image_wallpaper = create_image(NULL);
    widget_add( container, image_wallpaper, BRD_CENTER );
    widget_inc_ref( image_wallpaper ); /* reference for the global variable */

    /* Start the wallpaper handler thread. */

    wallpaper_start();

    /* Set the default wallpaper according to the configuration. */

    if ( ycfg_get_ascii_value( "application/desktop/background", "file", &img_file ) == 0 ) {
        wallpaper_set(img_file);
        free(img_file);
    }

    /* Make the desktop window visible. */

    window_show( window );

    /* Send a notification to the GUI server that the desktop application is running. */

    send_guiserver_notification();
    application_set_message_handler( desktop_msg_handler );

    /* Start the mainloop of the application ... */

    application_run();

    return EXIT_SUCCESS;
}
