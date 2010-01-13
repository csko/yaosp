/* Taskbar application
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
#include <stdio.h>

#include <ygui/bitmap.h>
#include <ygui/menuitem.h>
#include <yconfig/yconfig.h>

#include "taskbar.h"

typedef struct menu_item {
    uint64_t position;
    char* name;
    char* executable;
} menu_item_t;

int taskbar_create_menu( void ) {
    int i;
    int size;
    array_t menu_items;

    taskbar_menu = create_menu();

    if ( init_array( &menu_items ) != 0 ) {
        return -1;
    }

    if ( ycfg_list_children( "application/taskbar/menu", &menu_items ) != 0 ) {
        destroy_array( &menu_items );
        return -1;
    }

    size = array_get_size( &menu_items );

    for ( i = 0; i < size; i++ ) {
        char* name;
        char path[ 256 ];
        menu_item_t* item;

        widget_t* menuitem;

        item = ( menu_item_t* )malloc( sizeof( menu_item_t ) );

        if ( item == NULL ) {
            continue;
        }

        name = ( char* )array_get_item( &menu_items, i );

        snprintf( path, sizeof( path ), "application/taskbar/menu/%s", name );

        if ( ycfg_get_numeric_value( path, "position", &item->position ) != 0 ) {
            free( item );
            continue;
        }

        if ( ycfg_get_ascii_value( path, "name", &item->name ) != 0 ) {
            free( item );
            continue;
        }

        if ( ycfg_get_ascii_value( path, "executable", &item->executable ) != 0 ) {
            free( item->name );
            free( item );
            continue;
        }

        menuitem = create_menuitem_with_label( item->name );
        menu_add_item( taskbar_menu, menuitem );
    }

#if 0
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

    item = create_separator_menuitem();
    menu_add_item( menu, item );

    /* Shutdown */

    image = bitmap_load_from_file( "/application/taskbar/images/shutdown.png" );
    item = create_menuitem_with_label_and_image( "Shut down", image );
    menu_add_item( menu, item );
    bitmap_dec_ref( image );

    widget_connect_event_handler( item, "clicked", event_open_shutdown_window, NULL );
#endif

    return 0;
}
