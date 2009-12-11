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

#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/param.h>
#include <yaosp/debug.h>
#include <yutil/array.h>
#include <ygui/font.h>
#include <ygui/window.h>

#include "window_list.h"

#define W_WINDOW_LIST W_TYPE_COUNT

#define WL_ROWS 1
#define WL_MAX_ITEM_WIDTH 150

static array_t window_table;

static font_t* list_font = NULL;
static bitmap_t* unknown_app = NULL;

extern window_t* window;
extern widget_t* win_list_widget;

static window_item_t* window_item_create( msg_win_info_t* info ) {
    window_item_t* item;

    item = ( window_item_t* )malloc( sizeof( window_item_t ) );

    if ( item == NULL ) {
        goto error1;
    }

    item->title = strdup( ( const char* )( info + 1 ) );

    if ( item->title == NULL ) {
        goto error2;
    }

    if ( info->icon_bitmap == -1 ) {
        item->icon = NULL;
    } else {
        item->icon = bitmap_clone( info->icon_bitmap );
    }

    item->id = info->id;

    return item;

 error2:
    free( item );

 error1:
    return NULL;
}

static int window_item_free( window_item_t* item ) {
    if ( item->icon != NULL ) {
        bitmap_dec_ref( item->icon );
    }

    free( item->title );
    free( item );

    return 0;
}

static int window_list_add( void* data ) {
    array_add_item( &window_table, data );
    widget_invalidate( win_list_widget );

    return 0;
}

static int window_list_remove( void* data ) {
    int i;
    int id;
    int size;

    id = *( int* )data;
    free( data );

    size = array_get_size( &window_table );

    for ( i = 0; i < size; i++ ) {
        window_item_t* item;

        item = ( window_item_t* )array_get_item( &window_table, i );

        if ( item->id == id ) {
            array_remove_item_from( &window_table, i );
            window_item_free( item );

            widget_invalidate( win_list_widget );

            return 0;
        }
    }

    return -EINVAL;
}

int taskbar_handle_window_list( uint8_t* data ) {
    int i;
    int win_count;
    int title_len;
    msg_win_info_t* win_info;

    win_count = *( int* )data;
    data += sizeof( int );

    for ( i = 0; i < win_count; i++ ) {
        window_item_t* win_item;

        win_info = ( msg_win_info_t* )data;
        title_len = strlen( ( const char* )( win_info + 1 ) );

        win_item = window_item_create( win_info );

        if ( win_item != NULL ) {
            window_insert_callback( window, window_list_add, ( void* )win_item );
        }

        data += sizeof( msg_win_info_t );
        data += title_len + 1;
    }

    return 0;
}

int taskbar_handle_window_opened( msg_win_info_t* win_info ) {
    window_item_t* win_item;

    win_item = window_item_create( win_info );

    if ( win_item != NULL ) {
        window_insert_callback( window, window_list_add, ( void* )win_item );
    }

    return 0;
}

int taskbar_handle_window_closed( msg_win_info_t* win_info ) {
    int* id = ( int* )malloc( sizeof( int ) );

    if ( id != NULL ) {
        *id = win_info->id;
        window_insert_callback( window, window_list_remove, ( void* )id );
    }

    return 0;
}

static int window_list_paint( widget_t* widget, gc_t* gc ) {
    int i;
    int size;
    rect_t bounds;
    rect_t item_rect;
    int item_width;
    int item_height;
    int current_row;
    int items_per_row;

    static color_t black = { 0, 0, 0, 255 };
    static color_t background = { 216, 216, 216, 255 };
    static color_t darker_bg = { 181, 181, 181, 255 };

    widget_get_bounds( widget, &bounds );

    gc_set_pen_color( gc, &background );
    gc_fill_rect( gc, &bounds );

    item_height = ( rect_height( &bounds ) - ( WL_ROWS + 1 ) ) / WL_ROWS;

    gc_set_font( gc, list_font );

    size = array_get_size( &window_table );
    items_per_row = size / WL_ROWS;

    if ( items_per_row == 0 ) {
        item_width = WL_MAX_ITEM_WIDTH;
    } else {
        item_width = ( rect_width( &bounds ) - ( items_per_row + 1 ) ) / items_per_row;
        item_width = MIN( item_width, WL_MAX_ITEM_WIDTH );
    }

    current_row = 0;
    item_rect.left = 1;
    item_rect.top = 1;

    for ( i = 0; i < size; i++ ) {
        point_t position;
        window_item_t* win_item;

        win_item = ( window_item_t* )array_get_item( &window_table, i );

        item_rect.right = item_rect.left + item_width - 1;
        item_rect.bottom = item_rect.top + item_height - 1;

        gc_set_pen_color( gc, &black );
        gc_draw_rect( gc, &item_rect );

        rect_resize( &item_rect, 1, 1, -1, -1 );

        gc_set_pen_color( gc, &darker_bg );
        gc_fill_rect( gc, &item_rect );

        gc_set_pen_color( gc, &black );
        gc_set_clip_rect( gc, &item_rect );

        point_init(
            &position,
            item_rect.left + 1,
            item_rect.top + 1
        );

        gc_set_drawing_mode( gc, DM_BLEND );

        if ( win_item->icon != NULL ) {
            gc_draw_bitmap( gc, &position, win_item->icon );
            position.x += bitmap_get_width( win_item->icon ) + 1;
        } else {
            gc_draw_bitmap( gc, &position, unknown_app );
            position.x += bitmap_get_width( unknown_app ) + 1;
        }

        gc_set_drawing_mode( gc, DM_COPY );

        position.y = item_rect.bottom - ( 1 + font_get_line_gap( list_font ) - font_get_descender( list_font ) );
        gc_draw_text( gc, &position, win_item->title, -1 );

        gc_reset_clip_rect( gc );

        rect_resize( &item_rect, -1, -1, 1, 1 );

        if ( ++current_row == WL_ROWS ) {
            current_row = 0;

            item_rect.left += item_width + 1;
            item_rect.top = 1;
        } else {
            item_rect.top = item_rect.bottom + 2;
        }
    }

    return 0;
}

static int window_list_mouse_pressed( widget_t* widget, point_t* position, int button ) {
    int size;
    int real_x;
    int real_y;
    int item_width;
    int item_height;
    int items_per_row;
    int index;
    rect_t bounds;
    window_item_t* win_item;

    widget_get_bounds( widget, &bounds );

    size = array_get_size( &window_table );
    items_per_row = size / WL_ROWS;

    item_height = ( rect_height( &bounds ) - ( WL_ROWS + 1 ) ) / WL_ROWS;

    if ( items_per_row == 0 ) {
        item_width = WL_MAX_ITEM_WIDTH;
    } else {
        item_width = ( rect_width( &bounds ) - ( items_per_row + 1 ) ) / items_per_row;
        item_width = MIN( item_width, WL_MAX_ITEM_WIDTH );
    }

    real_x = position->x - ( position->x / item_width + 1 );
    real_y = position->y - ( position->y / item_height + 1 );

    index = real_x / item_width * WL_ROWS + real_y / item_height;

    if ( index >= array_get_size( &window_table ) ) {
        return 0;
    }

    win_item = ( window_item_t* )array_get_item( &window_table, index );
    window_bring_to_front( win_item->id );

    return 0;
}

static widget_operations_t win_list_ops = {
    .paint = window_list_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = window_list_mouse_pressed,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = NULL,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL
};

widget_t* window_list_create( void ) {
    font_properties_t properties;

    if ( init_array( &window_table ) != 0 ) {
        goto error1;
    }

    array_set_realloc_size( &window_table, 32 );

    properties.point_size = 8 * 64;
    properties.flags = FONT_SMOOTHED;

    list_font = create_font( "DejaVu Sans", "Book", &properties );

    if ( list_font == NULL ) {
        goto error2;
    }

    unknown_app = bitmap_load_from_file( "/application/taskbar/images/unknown.png" );

    if ( unknown_app == NULL ) {
        goto error3;
    }

    return create_widget( W_WINDOW_LIST, &win_list_ops, NULL );

 error3:
    destroy_font( list_font );

 error2:
    destroy_array( &window_table );

 error1:
    return NULL;
}
