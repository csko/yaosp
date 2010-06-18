/* Desktop application
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

#ifndef _WALLPAPER_H_
#define _WALLPAPER_H_

#include <ygui/point.h>

extern point_t wallpaper_size;

int wallpaper_set( char* filename );
int wallpaper_resize( int width, int height );

int wallpaper_init( void );
int wallpaper_start( void );

#endif /* _WALLPAPER_H_ */
