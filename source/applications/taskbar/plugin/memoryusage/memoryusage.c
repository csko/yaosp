/* Memory usage plugin for the taskbar application
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

#include <time.h>
#include <stdio.h>
#include <yaosp/sysinfo.h>

#include <ygui/font.h>
#include <ygui/window.h>

#include "../../plugin.h"

#define W_MEMORYUSAGE W_TYPE_COUNT + 3

static double current_usage;

static int do_update_current_usage( void ) {
    memory_info_t info;

    get_memory_info( &info );
    current_usage = ( double )( info.total_page_count - info.free_page_count ) / info.total_page_count;

    return 0;
}

static int memoryusage_update_callback( window_t* window, void* data ) {
    widget_t* widget;

    do_update_current_usage();

    widget = ( widget_t* )data;
    widget_invalidate( widget );

    return 0;
}

static int memoryusage_paint( widget_t* widget, gc_t* gc ) {
    rect_t tmp;
    rect_t bounds;

    static color_t bg = { 0, 0, 0, 255 };
    static color_t fg = { 192, 192, 192, 255 };

    widget_get_bounds( widget, &bounds );
    rect_resize( &bounds, 1, 1, -1, -1 );

    gc_set_pen_color( gc, &bg );
    gc_fill_rect( gc, &bounds );

    rect_resize( &bounds, 1, 1, -1, -1 );

    rect_init(
        &tmp,
        bounds.left,
        bounds.bottom - ( int )( ( rect_height( &bounds ) * current_usage ) ) + 1,
        bounds.right,
        bounds.bottom
    );

    gc_set_pen_color( gc, &fg );
    gc_fill_rect( gc, &tmp );

    return 0;
}

static int memoryusage_get_preferred_size( widget_t* widget, point_t* size ) {
    point_init( size, 7, -1 );

    return 0;
}

static int memoryusage_added_to_window( widget_t* widget ) {
    window_insert_timer(
        widget->window,
        1 * 1000 * 1000,
        1,
        memoryusage_update_callback,
        ( void* )widget
    );

    return 0;
}

static widget_operations_t memoryusage_ops = {
    .paint = memoryusage_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = memoryusage_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = memoryusage_added_to_window,
    .child_added = NULL
};

static widget_t* memoryusage_create( void ) {
    do_update_current_usage();

    return create_widget( W_MEMORYUSAGE, &memoryusage_ops, NULL );
}

taskbar_plugin_t memoryusage_plugin = {
    .name = "Memory usage",
    .create = memoryusage_create
};
