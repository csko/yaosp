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
#include <yaosp/ipc.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/textfield.h>
#include <ygui/button.h>
#include <ygui/desktop.h>
#include <ygui/image.h>
#include <ygui/menu.h>
#include <ygui/menuitem.h>
#include <ygui/protocol.h>
#include <ygui/layout/borderlayout.h>
#include <ygui/layout/flowlayout.h>

#include "window_list.h"
#include "plugin.h"

window_t* window;
point_t win_lefttop;
widget_t* win_list_widget;

static menu_t* menu;

extern ipc_port_id guiserver_port;
extern taskbar_plugin_t datetime_plugin;
extern taskbar_plugin_t cpuload_plugin;
extern taskbar_plugin_t memoryusage_plugin;

static taskbar_plugin_t* plugins[] = {
    &cpuload_plugin,
    &memoryusage_plugin,
    &datetime_plugin,
    NULL
};

static int taskbar_msg_handler( uint32_t code, void* data ) {
    switch ( code ) {
        case MSG_WINDOW_LIST :
            taskbar_handle_window_list( ( uint8_t* )data );
            break;

        case MSG_WINDOW_OPENED :
            taskbar_handle_window_opened( ( msg_win_info_t* )data );
            break;

        case MSG_WINDOW_CLOSED :
            taskbar_handle_window_closed( ( msg_win_info_t* )data );
            break;

        default :
            return -1;
    }

    return 0;
}

static int event_open_terminal( widget_t* widget, void* data ) {
    if ( fork() == 0 ) {
        char* argv[] = { "terminal", NULL };
        execv( "/application/terminal/terminal", argv );
        _exit( 1 );
    }

    return 0;
}

static int event_open_texteditor( widget_t* widget, void* data ) {
    if ( fork() == 0 ) {
        char* argv[] = { "texteditor", NULL };
        execv( "/application/texteditor/texteditor", argv );
        _exit( 1 );
    }

    return 0;
}

static int event_open_taskbar( widget_t* widget, void* data ) {
    point_t size;
    point_t position;

    menu_get_size( menu, &size );

    position.x = 0;
    position.y = win_lefttop.y - size.y;

    menu_popup_at( menu, &position );

    return 0;
}

static int taskbar_create_menu( void ) {
    widget_t* item;
    bitmap_t* image;

    menu = create_menu();

    /* Terminal */

    image = bitmap_load_from_file( "/application/taskbar/images/terminal.png" );
    item = create_menuitem_with_label_and_image( "Terminal", image );
    menu_add_item( menu, item );
    bitmap_dec_ref( image );

    widget_connect_event_handler( item, "clicked", event_open_terminal, NULL );

    /* Text editor */

    image = bitmap_load_from_file( "/application/taskbar/images/texteditor.png" );
    item = create_menuitem_with_label_and_image( "Text editor", image );
    menu_add_item( menu, item );
    bitmap_dec_ref( image );

    widget_connect_event_handler( item, "clicked", event_open_texteditor, NULL );

    return 0;
}

static int send_guiserver_notification( void ) {
    send_ipc_message( guiserver_port, MSG_TASKBAR_STARTED, NULL, 0 );

    return 0;
}

int main( int argc, char** argv ) {
    int i;
    point_t size;

    if ( application_init() != 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return EXIT_FAILURE;
    }

    desktop_get_size( &size );

    win_lefttop.x = 0;
    win_lefttop.y = size.y - 22;

    size.y = 22;

    /* Create a window */

    window = create_window( "Taskbar", &win_lefttop, &size, WINDOW_NO_BORDER );

    widget_t* container = window_get_container( window );

    /* Set the layout of the window */

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    widget_t* image = create_image( bitmap_load_from_file( "/application/taskbar/images/start.png" ) );
    widget_add( container, image, BRD_LINE_START );
    widget_dec_ref( image );

    widget_connect_event_handler( image, "mouse-down", event_open_taskbar, NULL );

    win_list_widget = window_list_create();
    widget_add( container, win_list_widget, BRD_CENTER );
    widget_dec_ref( win_list_widget );

    widget_t* plugin_panel = create_panel();
    widget_add( container, plugin_panel, BRD_LINE_END );

    layout = create_flow_layout();
    panel_set_layout( plugin_panel, layout );
    layout_dec_ref( layout );

    for ( i = 0; plugins[ i ] != NULL; i++ ) {
        taskbar_plugin_t* plugin = plugins[ i ];
        widget_t* plugin_widget = plugin->create();

        widget_add( plugin_panel, plugin_widget, 0 );
        widget_dec_ref( plugin_widget );
    }

    widget_dec_ref( plugin_panel );

    /* Create the menu */

    taskbar_create_menu();

    /* Show the window */

    window_show( window );

    /* Register taskbar application for listening window list events */

    application_set_message_handler( taskbar_msg_handler );
    application_register_window_listener( 1 );

    /* Tell the GUI server that the taskbar is running */

    send_guiserver_notification();

    /* Start the mainloop of the application ... */

    application_run();

    return EXIT_SUCCESS;
}
