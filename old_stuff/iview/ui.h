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

#ifndef _UI_H_
#define _UI_H_

#include <ygui/window.h>
#include <ygui/image.h>

extern window_t* window;
extern widget_t* image_widget;

extern point_t current_size;
extern bitmap_t* orig_bitmap;

int ui_set_statusbar( const char* format, ... );
int ui_set_image_info( const char* format, ... );

int ui_init( void );

#endif /* _UI_H_ */
