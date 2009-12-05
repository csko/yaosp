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

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/textarea.h>
#include <ygui/menubar.h>
#include <ygui/menuitem.h>
#include <ygui/layout/borderlayout.h>
#include <ygui/dialog/filechooser.h>

static int event_open_file( widget_t* widget, void* data ) {
    file_chooser_t* chooser = create_file_chooser( T_OPEN_DIALOG, "/" );
    file_chooser_show( chooser );

    return 0;
}

int main( int argc, char** argv ) {
    if ( application_init() != 0 ) {
        return EXIT_FAILURE;
    }

    point_t pos = { 75, 75 };
    point_t size = { 300, 300 };

    window_t* window = create_window( "Text editor", &pos, &size, WINDOW_NONE );

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

    widget_t* textarea = create_textarea();
    widget_add( container, textarea, BRD_CENTER );
    widget_dec_ref( textarea );

    window_show( window );

    /* The mainloop of the application ... */

    application_run();

    return EXIT_SUCCESS;
}
