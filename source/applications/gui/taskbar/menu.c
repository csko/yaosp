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
#include <unistd.h>

#include <ygui/bitmap.h>
#include <ygui/menuitem.h>
#include <yconfig/yconfig.h>

#include "taskbar.h"

typedef struct menu_item {
    uint64_t position;
    char* name;
    char* executable;
    bitmap_t* image;
} menu_item_t;

static int event_taskbar_item_clicked( widget_t* widget, void* data ) {
    menu_item_t* item;

    item = ( menu_item_t* )widget_get_private( widget );

    if ( fork() == 0 ) {
        char* argv[] = { strrchr( item->executable, '/' ) + 1, NULL };
        execv( item->executable, argv );
        _exit( EXIT_FAILURE );
    }

    return 0;
}

static int taskbar_menu_comparator( const void* key1, const void* key2 ) {
    menu_item_t* item1 = *( menu_item_t** )key1;
    menu_item_t* item2 = *( menu_item_t** )key2;

    if ( item1->position < item2->position ) {
        return -1;
    } else if ( item1->position > item2->position ) {
        return 1;
    }

    return 0;
}

int taskbar_create_menu( void ) {
    int i;
    int size;
    array_t menu_items;
    array_t menu_item_names;

    taskbar_menu = create_menu();

    if ( init_array( &menu_item_names ) != 0 ) {
        return -1;
    }

    if ( init_array( &menu_items ) != 0 ) {
        return -1;
    }

    if ( ycfg_list_children( "application/taskbar/menu", &menu_item_names ) != 0 ) {
        destroy_array( &menu_item_names );
        return -1;
    }

    size = array_get_size( &menu_item_names );

    for ( i = 0; i < size; i++ ) {
        char* name;
        char path[ 256 ];
        menu_item_t* item;

        void* image_data;
        size_t image_size;

        item = ( menu_item_t* )malloc( sizeof( menu_item_t ) );

        if ( item == NULL ) {
            continue;
        }

        name = ( char* )array_get_item( &menu_item_names, i );

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

        if ( ycfg_get_binary_value( path, "image", &image_data, &image_size ) == 0 ) {
            item->image = bitmap_load_from_buffer( image_data, image_size );
            free( image_data );
        } else {
            item->image = NULL;
        }

        array_add_item( &menu_items, item );
    }

    array_sort( &menu_items, taskbar_menu_comparator );

    for ( i = 0; i < size; i++ ) {
        menu_item_t* item;
        widget_t* menuitem;

        item = ( menu_item_t* )array_get_item( &menu_items, i );

        menuitem = create_menuitem_with_label_and_image( item->name, item->image );
        menu_add_item( taskbar_menu, menuitem );

        widget_set_private( menuitem, item );
        widget_connect_event_handler( menuitem, "clicked", event_taskbar_item_clicked, NULL );
    }

    return 0;
}
