/* yaosp GUI library
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

#ifndef _YGUI_WINDOW_H_
#define _YGUI_WINDOW_H_

#include <pthread.h>
#include <sys/types.h>

#include <yaosp/ipc.h>

#include <ygui/point.h>
#include <ygui/widget.h>
#include <ygui/gc.h>
#include <yutil/array.h>

enum {
    WE_ACTIVATED = 0,
    WE_DEACTIVATED,
    WE_CLOSED,
    WE_COUNT
};

enum {
    WINDOW_HIDE = 0,
    WINDOW_DESTROY
};

struct window;

typedef int window_event_callback_t( struct window* window, void* data );

typedef struct window_event_data {
    window_event_callback_t* callback;
    void* data;
} window_event_data_t;

typedef struct window_timer {
    uint64_t expire_time;
    uint64_t timeout;
    int periodic;
    window_event_callback_t* callback;
    void* data;
} window_timer_t;

typedef struct window {
    /* Window IPC ports */

    pthread_t thread;
    ipc_port_id client_port;
    ipc_port_id server_port;
    ipc_port_id reply_port;

    /* Widget management */

    point_t position;
    widget_t* container;
    widget_t* focused_widget;
    widget_t* mouse_widget;
    widget_t* mouse_down_widget;

    /* Event handling */

    int close_operation;
    window_event_data_t event_handlers[ WE_COUNT ];

    array_t timers;
    pthread_mutex_t timer_lock;

    /* Rendering informations */

    gc_t gc;
    uint8_t* render_buffer;
    size_t render_buffer_size;
    size_t render_buffer_max_size;
} window_t;

typedef int window_callback_t( void* data );

typedef struct window_callback_item {
    window_callback_t* callback;
    void* data;
} window_callback_item_t;

enum {
    WINDOW_NONE = 0,
    WINDOW_NO_BORDER = ( 1 << 0 )
};

widget_t* window_get_container( window_t* window );
int window_get_position( window_t* window, point_t* position );

/* Window callback handling */

int window_set_event_handler( window_t* window, int event, window_event_callback_t* callback, void* data );

int window_insert_callback( window_t* window, window_callback_t* callback, void* data );
int window_insert_timer( window_t* window, uint64_t timeout, int periodic, window_event_callback_t* callback, void* data );

window_t* create_window( const char* title, point_t* position, point_t* size, int flags );

int window_resize( window_t* window, point_t* size );
int window_move( window_t* window, point_t* position );

int window_set_icon( window_t* window, bitmap_t* bitmap );

int window_show( window_t* window );
int window_hide( window_t* window );
int window_close( window_t* window );

int window_bring_to_front( int window_id );

#endif /* _YGUI_WINDOW_H_ */
