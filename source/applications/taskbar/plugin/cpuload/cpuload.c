/* CPU load plugin for the taskbar application
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
#include <yaosp/time.h>

#include <ygui/font.h>
#include <ygui/window.h>

#include "../../plugin.h"

#define W_CPULOAD W_TYPE_COUNT + 2

static uint64_t prev_idle;
static uint64_t prev_time;
static double current_load;

static int cpuload_update_callback( window_t* window, void* data ) {
    widget_t* widget;

    uint64_t cur_idle;
    uint64_t cur_time;

    cur_idle = get_idle_time();
    cur_time = get_system_time();

    current_load = 1.0f - ( ( double )( cur_idle - prev_idle ) / ( cur_time - prev_time ) );

    prev_idle = cur_idle;
    prev_time = cur_time;

    widget = ( widget_t* )data;
    widget_invalidate( widget );

    return 0;
}

static int cpuload_paint( widget_t* widget, gc_t* gc ) {
    rect_t tmp;
    rect_t bounds;

    static color_t bg = { 0, 0, 0, 255 };
    static color_t fg = { 128, 128, 128, 255 };

    widget_get_bounds( widget, &bounds );
    rect_resize( &bounds, 1, 1, -1, -1 );

    gc_set_pen_color( gc, &bg );
    gc_fill_rect( gc, &bounds );

    rect_resize( &bounds, 1, 1, -1, -1 );

    rect_init(
        &tmp,
        bounds.left,
        bounds.bottom - ( int )( ( rect_height( &bounds ) * current_load ) ) + 1,
        bounds.right,
        bounds.bottom
    );

    gc_set_pen_color( gc, &fg );
    gc_fill_rect( gc, &tmp );

    return 0;
}

static int cpuload_get_preferred_size( widget_t* widget, point_t* size ) {
    point_init( size, 7, -1 );

    return 0;
}

static int cpuload_added_to_window( widget_t* widget ) {
    window_insert_timer(
        widget->window,
        500 * 1000,
        1,
        cpuload_update_callback,
        ( void* )widget
    );

    return 0;
}

static widget_operations_t cpuload_ops = {
    .paint = cpuload_paint,
    .key_pressed = NULL,
    .key_released = NULL,
    .mouse_entered = NULL,
    .mouse_exited = NULL,
    .mouse_moved = NULL,
    .mouse_pressed = NULL,
    .mouse_released = NULL,
    .get_minimum_size = NULL,
    .get_preferred_size = cpuload_get_preferred_size,
    .get_maximum_size = NULL,
    .do_validate = NULL,
    .size_changed = NULL,
    .added_to_window = cpuload_added_to_window,
    .child_added = NULL
};

static widget_t* cpuload_create( void ) {
    prev_idle = get_idle_time();
    prev_time = get_system_time();

    current_load = 0.0f;

    return create_widget( W_CPULOAD, &cpuload_ops, NULL );
}

taskbar_plugin_t cpuload_plugin = {
    .name = "CPU load",
    .create = cpuload_create
};
