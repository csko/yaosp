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
#include <yaosp/debug.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/textfield.h>
#include <ygui/button.h>
#include <ygui/desktop.h>
#include <ygui/scrollpanel.h>
#include <ygui/layout/borderlayout.h>

static int event_button_clicked( widget_t* widget, void* data ) {
    window_t* win;

    point_t point = {
        .x = random() % 100,
        .y = random() % 100
    };
    point_t size = {
        .x = 100 + random() % 200,
        .y = 100 + random() % 200
    };

    win = create_window( "Test window", &point, &size, 0 );

    widget_t* container = window_get_container( win );

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    widget_t* scroll = create_scroll_panel( SCROLLBAR_ALWAYS, SCROLLBAR_ALWAYS );
    widget_add( container, scroll, BRD_CENTER );
    widget_dec_ref( scroll );

    widget_t* button = create_button( "Hello World!" );
    point_t pref_size = { .x = 600, .y = 600 };
    widget_set_preferred_size( button, &pref_size );
    widget_add( scroll, button, NULL );
    widget_dec_ref( button );

    show_window( win );

    return 0;
}

static int event_textfield_activated( widget_t* widget, void* data ) {
    char* text;

    text = textfield_get_text( widget );

    if ( text != NULL ) {
        dbprintf( "Textfield data: '%s'\n", text );

        free( text );
    }

    textfield_set_text( widget, NULL );

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

    /* Create a test label */

    widget_t* button = create_button( "Press me!" );
    widget_add( container, button, BRD_PAGE_END );
    widget_dec_ref( button );

    widget_t* textfield = create_textfield();
    widget_add( container, textfield, BRD_CENTER );
    widget_dec_ref( textfield );

    widget_connect_event_handler( button, "clicked", event_button_clicked, NULL );
    widget_connect_event_handler( textfield, "activated", event_textfield_activated, NULL );

    /* Show the window */

    show_window( win );

    run_application();

    return 0;
}
