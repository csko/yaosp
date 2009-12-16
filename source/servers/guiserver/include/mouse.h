/* GUI server
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

#ifndef _GUISERVER_MOUSE_H_
#define _GUISERVER_MOUSE_H_

#include <sys/types.h>
#include <ygui/rect.h>
#include <ygui/point.h>
#include <yutil/hashtable.h>

#include <bitmap.h>

typedef int mouse_pointer_id;

typedef struct mouse_pointer {
    hashitem_t hash;

    mouse_pointer_id id;
    int ref_count;

    bitmap_t* pointer_bitmap;
    bitmap_t* hidden_bitmap;
} mouse_pointer_t;

mouse_pointer_t* create_mouse_pointer( uint32_t width, uint32_t height, color_space_t color_space, void* raster );
int put_mouse_pointer( mouse_pointer_t* pointer );

int activate_mouse_pointer( mouse_pointer_t* pointer );

int show_mouse_pointer( void );
int hide_mouse_pointer( void );

int mouse_moved( point_t* delta );
int mouse_get_position( point_t* position );
int mouse_get_rect( rect_t* mouse_rect );

int init_mouse_manager( void );

#endif /* _GUISERVER_MOUSE_H_ */
