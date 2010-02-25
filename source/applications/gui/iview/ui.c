/* Image viewer application
 *
 * Copyright (c) 2010 Attila Magyar, Zoltan Kovacs
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
#include <stdarg.h>
#include <stdio.h>

#include <ygui/panel.h>
#include <ygui/desktop.h>
#include <ygui/menubar.h>
#include <ygui/menuitem.h>
#include <ygui/scrollpanel.h>
#include <ygui/label.h>
#include <ygui/layout/borderlayout.h>
#include <ygui/dialog/filechooser.h>

#include "ui.h"
#include "ui-event.h"
#include "worker.h"

window_t* window;
widget_t* image_widget;
static widget_t* label_statusbar;
static widget_t* label_image_info;

point_t current_size;
bitmap_t* orig_bitmap;

static int statusbar_update_callback( void* data ) {
    char* text = (char*)data;

    label_set_text(label_statusbar, text);
    free(text);

    return 0;
}

int ui_set_statusbar( const char* format, ... ) {
    char buf[512];
    va_list ap;

    va_start(ap, format);
    vsnprintf( buf, sizeof(buf), format, ap );
    va_end(ap);

    window_insert_callback( window, statusbar_update_callback, strdup(buf) );

    return 0;
}

static int image_info_update_callback( void* data ) {
    char* text = (char*)data;

    label_set_text(label_image_info, text);
    free(text);

    return 0;
}

int ui_set_image_info( const char* format, ... ) {
    char buf[512];
    va_list ap;

    va_start(ap, format);
    vsnprintf( buf, sizeof(buf), format, ap );
    va_end(ap);

    window_insert_callback( window, image_info_update_callback, strdup(buf) );

    return 0;
}

static int menu_init( widget_t* container ) {
    widget_t* menubar = create_menubar();
    widget_add( container, menubar, BRD_PAGE_START );

    // File menu

    widget_t* item = create_menuitem_with_label( "File" );
    menubar_add_item( menubar, item );

    menu_t* menu = create_menu();
    menuitem_set_submenu( item, menu );

    item = create_menuitem_with_label( "Open" );
    menu_add_item( menu, item );
    widget_connect_event_handler( item, "mouse-down", event_open_file, NULL );

    item = create_separator_menuitem();
    menu_add_item( menu, item );

    item = create_menuitem_with_label( "Exit" );
    menu_add_item( menu, item );
    widget_connect_event_handler( item, "mouse-down", event_application_exit, NULL );

    // View menu

    item = create_menuitem_with_label( "View" );
    menubar_add_item( menubar, item );

    menu = create_menu();
    menuitem_set_submenu( item, menu );

    item = create_menuitem_with_label( "Zoom in" );
    menu_add_item( menu, item );
    widget_connect_event_handler( item, "mouse-down", event_zoom_in, NULL );

    item = create_menuitem_with_label( "Zoom out" );
    menu_add_item( menu, item );
    widget_connect_event_handler( item, "mouse-down", event_zoom_out, NULL );

    item = create_menuitem_with_label( "Fit to screen" );
    menu_add_item( menu, item );

    // Help menu

    item = create_menuitem_with_label( "Help" );
    menubar_add_item( menubar, item );

    menu = create_menu();
    menuitem_set_submenu( item, menu );

    item = create_menuitem_with_label( "About" );
    menu_add_item( menu, item );

    return 0;
}

int ui_init( void ) {
    point_t pos;
    point_t size;

    point_init( &pos, 0, 0 );
    point_init( &size, 480, 350 );

    window = create_window( "iView", &pos, &size, W_ORDER_NORMAL, WINDOW_NONE );

    bitmap_t* icon = bitmap_load_from_file( "/application/iview/images/iview_16x16.png" );
    window_set_icon( window, icon );
    bitmap_dec_ref( icon );

    widget_t* container = window_get_container( window );

    layout_t* layout = create_borderlayout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    menu_init(container);

    widget_t* scrollpanel = create_scroll_panel( SCROLLBAR_ALWAYS, SCROLLBAR_ALWAYS );
    widget_add( container, scrollpanel, BRD_CENTER );

    image_widget = create_image(NULL);
    widget_add( scrollpanel, image_widget, NULL );

    widget_t* panel = create_panel();
    widget_add( container, panel, BRD_PAGE_END );

    layout = create_borderlayout();
    panel_set_layout( panel, layout );
    layout_dec_ref( layout );

    label_statusbar = create_label();
    label_set_horizontal_alignment( label_statusbar, H_ALIGN_LEFT );
    widget_add( panel, label_statusbar, BRD_CENTER );

    label_image_info = create_label();
    label_set_horizontal_alignment( label_image_info, H_ALIGN_RIGHT );

    point_init( &size, 120, 0 );
    widget_set_preferred_size( label_image_info, &size );

    widget_add( panel, label_image_info, BRD_LINE_END );

    return 0;
}
