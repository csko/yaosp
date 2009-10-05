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
#include <ygui/layout/borderlayout.h>

static widget_t* textfield;

static int event_button_clicked( widget_t* widget, void* data ) {
    char* text;

    text = textfield_get_text( textfield );

    if ( text != NULL ) {
        if ( fork() == 0 ) {
            char* argv[] = { text, NULL };

            execv( text, argv );
            _exit( 1 );
        }

        free( text );
    }

    textfield_set_text( textfield, NULL );

    return 0;
}

#include <ygui/bitmap.h>
static void test_png_stuff( void ) {
    return;
    bitmap_t* bmp = bitmap_load_from_file( "/system/test.png" );
    dbprintf( "%s(): bmp = %p\n", __FUNCTION__, bmp );
}

int main( int argc, char** argv ) {
    int error;
    window_t* win;

    error = create_application();

    if ( error < 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return error;
    }

    // ---
    test_png_stuff();
    // ---

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

    textfield = create_textfield();
    widget_add( container, textfield, BRD_CENTER );
    widget_dec_ref( textfield );

    widget_connect_event_handler( button, "clicked", event_button_clicked, NULL );

    /* Show the window */

    show_window( win );

    run_application();

    return 0;
}
