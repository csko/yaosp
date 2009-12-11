/* Text editor application
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

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/textarea.h>
#include <ygui/menubar.h>
#include <ygui/menuitem.h>
#include <ygui/scrollpanel.h>
#include <ygui/layout/borderlayout.h>
#include <ygui/dialog/filechooser.h>

#include <yutil/string.h>
#include <yutil/array.h>

static window_t* window;
static widget_t* textarea;

static int file_loader_insert_lines( void* data ) {
    array_t* lines;

    lines = ( array_t* )data;
    textarea_set_lines( textarea, lines );

    destroy_array( lines );
    free( lines );

    return 0;
}

static void* file_loader_thread( void* arg ) {
    int f;
    int ret;
    char* file;
    char tmp[ 8192 ];

    char* input = NULL;
    size_t input_size = 0;

    array_t* lines = ( array_t* )malloc( sizeof( array_t ) );
    /* todo: error checking */
    init_array( lines );
    array_set_realloc_size( lines, 256 );

    file = ( char* )arg;
    f = open( file, O_RDONLY );

    if ( f < 0 ) {
        goto out;
    }

    do {
        ret = read( f, tmp, sizeof( tmp ) );

        if ( ret > 0 ) {
            size_t new_input_size = input_size + ret;

            input = ( char* )realloc( input, new_input_size + 1 );
            /* todo: error checking */

            memcpy( input + input_size, tmp, ret );
            input[ new_input_size ] = 0;

            input_size = new_input_size;

            char* end;
            char* start = input;

            while ( ( end = strchr( start, '\n' ) ) != NULL ) {
                *end = 0;

                string_t* line = ( string_t* )malloc( sizeof( string_t ) );
                /* todo: error checking */
                init_string_from_buffer( line, start, end - start );

                array_add_item( lines, ( void* )line );

                start = end + 1;
            }

            if ( start > input ) {
                size_t remaining_input = input_size - ( start - input );

                if ( remaining_input == 0 ) {
                    free( input );
                    input = NULL;
                } else {
                    input = ( char* )realloc( input, remaining_input + 1 );
                    /* todo: error checking */

                    input[ remaining_input ] = 0;
                }

                input_size = remaining_input;
            }
        }
    } while ( ret == sizeof( tmp ) );

    close( f );

 out:
    free( file );

    if ( array_get_size( lines ) > 0 ) {
        window_insert_callback( window, file_loader_insert_lines, ( void* )lines );
    } else {
        destroy_array( lines );
        free( lines );
    }

    return NULL;
}

static int event_file_chooser_done( file_chooser_t* chooser, chooser_event_t event, void* data ) {
    if ( event == E_CHOOSER_OK ) {
        char* file = file_chooser_get_selected_file( chooser );

        pthread_t thread;
        pthread_attr_t attr;

        pthread_attr_init( &attr );
        pthread_attr_setname( &attr, "file loader" );

        pthread_create(
            &thread, &attr,
            file_loader_thread, ( void* )file
        );

        pthread_attr_destroy( &attr );
    }

    return 0;
}

static int event_open_file( widget_t* widget, void* data ) {
    file_chooser_t* chooser;

    chooser = create_file_chooser( T_OPEN_DIALOG, "/", event_file_chooser_done, NULL );
    file_chooser_show( chooser );

    return 0;
}

static int event_application_exit( widget_t* widget, void* data ) {
    window_close( window );

    return 0;
}

int main( int argc, char** argv ) {
    if ( application_init() != 0 ) {
        return EXIT_FAILURE;
    }

    point_t pos = { 25, 25 };
    point_t size = { 450, 350 };

    window = create_window( "Text editor", &pos, &size, WINDOW_NONE );

    bitmap_t* icon = bitmap_load_from_file( "/application/texteditor/images/texteditor.png" );
    window_set_icon( window, icon );
    bitmap_dec_ref( icon );

    widget_t* container = window_get_container( window );

    /* Set the layout of the window */

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    widget_t* menubar = create_menubar();
    widget_add( container, menubar, BRD_PAGE_START );
    widget_dec_ref( menubar );

    /* File menu */

    widget_t* item = create_menuitem_with_label( "File" );
    menubar_add_item( menubar, item );
    widget_dec_ref( item );

    menu_t* menu = create_menu();
    menuitem_set_submenu( item, menu );

    item = create_menuitem_with_label( "Open" );
    menu_add_item( menu, item );
    widget_dec_ref( item );

    widget_connect_event_handler( item, "mouse-down", event_open_file, NULL );

    item = create_menuitem_with_label( "Save" );
    menu_add_item( menu, item );
    widget_dec_ref( item );

    item = create_separator_menuitem();
    menu_add_item( menu, item );
    widget_dec_ref( item );

    item = create_menuitem_with_label( "Exit" );
    menu_add_item( menu, item );
    widget_dec_ref( item );

    widget_connect_event_handler( item, "mouse-down", event_application_exit, NULL );

    /* Help menu */

    item = create_menuitem_with_label( "Help" );
    menubar_add_item( menubar, item );
    widget_dec_ref( item );

    menu = create_menu();
    menuitem_set_submenu( item, menu );

    item = create_menuitem_with_label( "About" );
    menu_add_item( menu, item );
    widget_dec_ref( item );

    /* Textarea ... */

    widget_t* scrollpanel = create_scroll_panel( SCROLLBAR_ALWAYS, SCROLLBAR_ALWAYS );
    widget_add( container, scrollpanel, BRD_CENTER );

    textarea = create_textarea();
    widget_add( scrollpanel, textarea, NULL );
    widget_dec_ref( textarea );
    widget_dec_ref( scrollpanel );

    window_show( window );

    /* The mainloop of the application ... */

    application_run();

    return EXIT_SUCCESS;
}
