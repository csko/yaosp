/* Shutdown application
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <stdlib.h>
#include <yaosp/yaosp.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/button.h>
#include <ygui/desktop.h>
#include <ygui/layout/flowlayout.h>
#include <ygui/layout/borderlayout.h>
#include <ygui/border/emptyborder.h>

static int event_halt_clicked( widget_t* widget, void* data ) {
    halt();
    return 0;
}

static int event_reboot_clicked( widget_t* widget, void* data ) {
    reboot();
    return 0;
}

static int init_ui( void ) {
    border_t* border;
    window_t* window;
    widget_t* button;
    layout_t* layout;
    widget_t* container;

    point_t size = { 120, 25 };
    point_t position;

    desktop_get_size( &position );
    position.x = ( position.x - size.x ) / 2;
    position.y = ( position.y - size.y ) / 2;

    window = create_window( "Shut down", &position, &size, W_ORDER_NORMAL, WINDOW_FIXED_SIZE );

    container = window_get_container( window );

    border = create_emptyborder( 3, 3, 3, 3 );
    widget_set_border( container, border );
    border_dec_ref(border);

    layout = create_flowlayout();
    panel_set_layout(container, layout);
    layout_dec_ref(layout);

    button = create_button( "  Halt  " );
    widget_add( container, button, NULL );
    widget_connect_event_handler( button, "clicked", event_halt_clicked, NULL );

    button = create_button( "  Reboot  " );
    widget_add( container, button, NULL );
    widget_connect_event_handler( button, "clicked", event_reboot_clicked, NULL );

    window_show( window );

    return 0;
}

int main( int argc, char** argv ) {
    if ( application_init( APP_NONE ) != 0 ) {
        return EXIT_FAILURE;
    }

    init_ui();

    application_run();

    return 0;
}
