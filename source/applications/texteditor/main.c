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
#include <errno.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/textarea.h>
#include <ygui/menubar.h>
#include <ygui/menuitem.h>
#include <ygui/scrollpanel.h>
#include <ygui/label.h>
#include <ygui/layout/borderlayout.h>
#include <ygui/dialog/filechooser.h>

#include <yutil/string.h>
#include <yutil/array.h>

#include "about.h"
#include "statusbar.h"
#include "io.h"

window_t* window;
widget_t* textarea;
widget_t* statusbar;

char* opened_file = NULL;

static int event_open_file_chooser_done( file_chooser_t* chooser, chooser_event_t event, void* data ) {
    if ( event == E_CHOOSER_OK ) {
        char* file;

        file = file_chooser_get_selected_file( chooser );
        io_load_file( file );
    }

    return 0;
}

static int file_save_callback( void* data ) {
    int f;
    char* file;

    file = ( char* )data;

    f = open( file, O_WRONLY | O_CREAT | O_TRUNC );

    if ( f < 0 ) {
        statusbar_set_text( "Failed to save '%s': %s.", file, strerror( errno ) );
        goto out;
    }

    int i;
    int line_count = textarea_get_line_count( textarea );

    for ( i = 0; i < line_count; i++ ) {
        char* line = textarea_get_line( textarea, i );

        write( f, line, strlen( line ) );
        write( f, "\n", 1 );

        free( line );
    }

    close( f );

    statusbar_set_text( "File '%s' saved.", file );

 out:
    free( file );

    return 0;
}

static int do_save_file( const char* file ) {
    window_insert_callback( window, file_save_callback, ( void* )file );

    return 0;
}

static int event_save_file_chooser_done( file_chooser_t* chooser, chooser_event_t event, void* data ) {
    if ( event == E_CHOOSER_OK ) {
        char* file = file_chooser_get_selected_file( chooser );

        do_save_file( file );
    }

    return 0;
}

static int event_open_file( widget_t* widget, void* data ) {
    file_chooser_t* chooser;

    chooser = create_file_chooser( T_OPEN_DIALOG, "/", event_open_file_chooser_done, NULL );
    file_chooser_show( chooser );

    return 0;
}

static int event_save_file( widget_t* widget, void* data ) {
    if ( opened_file == NULL ) {
        file_chooser_t* chooser;

        chooser = create_file_chooser( T_SAVE_DIALOG, "/", event_save_file_chooser_done, NULL );
        file_chooser_show( chooser );
    } else {
        do_save_file( strdup( opened_file ) );
    }

    return 0;
}

static int event_about_open( widget_t* widget, void* data ) {
    about_open();

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

    widget_connect_event_handler( item, "mouse-down", event_save_file, NULL );

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

    widget_connect_event_handler( item, "mouse-down", event_about_open, NULL );

    /* Textarea ... */

    widget_t* scrollpanel = create_scroll_panel( SCROLLBAR_ALWAYS, SCROLLBAR_ALWAYS );
    widget_add( container, scrollpanel, BRD_CENTER );

    textarea = create_textarea();
    widget_add( scrollpanel, textarea, NULL );
    widget_dec_ref( textarea );
    widget_dec_ref( scrollpanel );

    statusbar = create_label();
    label_set_horizontal_alignment( statusbar, H_ALIGN_LEFT );
    widget_add( container, statusbar, BRD_PAGE_END );
    widget_dec_ref( statusbar );

    window_show( window );

    /* The mainloop of the application ... */

    application_run();

    return EXIT_SUCCESS;
}
