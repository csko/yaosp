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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <yaosp/debug.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/image.h>
#include <ygui/desktop.h>
#include <ygui/menubar.h>
#include <ygui/menuitem.h>
#include <ygui/scrollpanel.h>
#include <ygui/label.h>
#include <ygui/layout/borderlayout.h>
#include <ygui/dialog/filechooser.h>

#include "worker.h"

static window_t* window;
static widget_t* image_widget;

static point_t current_size;
static bitmap_t* orig_bitmap;

static int image_set_callback( void* data ) {
    bitmap_t* bitmap = (bitmap_t*)data;

    image_set_bitmap( image_widget, bitmap );
    bitmap_dec_ref(bitmap);

    return 0;
}

static int image_open_done( work_header_t* work ) {
    open_work_t* open = (open_work_t*)work;

    if ( orig_bitmap != NULL ) {
        bitmap_dec_ref(orig_bitmap);
    }

    orig_bitmap = open->bitmap;

    bitmap_inc_ref(orig_bitmap);

    point_init(
        &current_size,
        bitmap_get_width(orig_bitmap),
        bitmap_get_height(orig_bitmap)
    );

    window_insert_callback( window, image_set_callback, (void*)orig_bitmap );

    return 0;
}

static int image_resize_done( work_header_t* work ) {
    resize_work_t* resize = (resize_work_t*)work;

    window_insert_callback( window, image_set_callback, (void*)resize->output );

    return 0;
}

static int event_open_file_chooser_done( file_chooser_t* chooser, chooser_event_t event, void* data ) {
    if ( event == E_CHOOSER_OK ) {
        open_work_t* work = (open_work_t*)malloc( sizeof(open_work_t) );

        work->header.type = OPEN_IMAGE;
        work->header.done = image_open_done;
        work->header.failed = NULL;
        work->path = file_chooser_get_selected_file( chooser );

        worker_put( (work_header_t*)work );
    }

    return 0;
}

static int event_open_file( widget_t* widget, void* data ) {
    file_chooser_t* chooser;

    chooser = create_file_chooser( T_OPEN_DIALOG, "/", event_open_file_chooser_done, NULL );
    file_chooser_show( chooser );

    return 0;
}

static int event_zoom_in( widget_t* widget, void* data ) {
    resize_work_t* work = (resize_work_t*)malloc( sizeof(resize_work_t) );

    work->header.type = RESIZE_IMAGE;
    work->header.done = image_resize_done;
    work->header.failed = NULL;

    work->input = orig_bitmap;
    point_add_xy( &current_size, 100, 100 );
    point_copy( &work->size, &current_size );

    worker_put( (work_header_t*)work );

    return 0;
}

static int event_zoom_out( widget_t* widget, void* data ) {
    resize_work_t* work;

    if ( ( current_size.x <= 100 ) ||
         ( current_size.y <= 100 ) ) {
        return 0;
    }

    work = (resize_work_t*)malloc( sizeof(resize_work_t) );

    work->header.type = RESIZE_IMAGE;
    work->header.done = image_resize_done;
    work->header.failed = NULL;

    work->input = orig_bitmap;

    point_sub_xy( &current_size, 100, 100 );
    point_copy( &work->size, &current_size );

    worker_put( (work_header_t*)work );

    return 0;
}

static int event_about_open( widget_t* widget, void* data ) {
    return 0;
}

static int event_application_exit( widget_t* widget, void* data ) {
    window_close( window );

    return 0;
}

static int initialize_menu( widget_t* container ) {
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
    widget_connect_event_handler( item, "mouse-down", event_about_open, NULL );

    return 0;
}

int main( int argc, char** argv ) {
    if ( application_init( APP_NONE ) != 0 ) {
        return EXIT_FAILURE;
    }

    point_t pos;
    point_t size;

    point_init( &pos, 0, 0 );
    point_init( &size, 320, 200 );

    window = create_window( "iView", &pos, &size, W_ORDER_NORMAL, WINDOW_NONE );

    widget_t* container = window_get_container( window );

    layout_t* layout = create_borderlayout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    initialize_menu(container);

    widget_t* scrollpanel = create_scroll_panel( SCROLLBAR_ALWAYS, SCROLLBAR_ALWAYS );
    widget_add( container, scrollpanel, BRD_CENTER );

    image_widget = create_image(NULL);
    widget_add( scrollpanel, image_widget, NULL );

    worker_init();
    worker_start();

    window_show( window );

    /* The mainloop of the application ... */

    application_run();

    return EXIT_SUCCESS;
}
