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

#ifndef _YGUI_BITMAP_H_
#define _YGUI_BITMAP_H_

#include <sys/types.h>
#include <yaosp/region.h>

#include <ygui/yconstants.h>

typedef struct bitmap {
    int id;
    int width;
    int height;
    color_space_t color_space;
    region_id region;
    uint8_t* data;
} bitmap_t;

bitmap_t* bitmap_create( int width, int height, color_space_t color_space );

bitmap_t* bitmap_load_from_file( const char* file );

#endif /* _YGUI_BITMAP_H_ */
