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

#ifndef _TASKBAR_WINDOW_LIST_H_
#define _TASKBAR_WINDOW_LIST_H_

#include <inttypes.h>

#include <ygui/protocol.h>
#include <ygui/widget.h>
#include <ygui/bitmap.h>

typedef struct window_item {
    int id;
    char* title;
    bitmap_t* icon;
} window_item_t;

int taskbar_handle_window_list( uint8_t* data );
int taskbar_handle_window_opened( msg_win_info_t* win_info );
int taskbar_handle_window_closed( msg_win_info_t* win_info );

widget_t* window_list_create( void );

#endif /* _TASKBAR_WINDOW_LIST_H_ */
