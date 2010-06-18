/* yaosp GUI library
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

#ifndef _YGUI_REGION_H_
#define _YGUI_REGION_H_

#include <ygui/rect.h>

typedef struct clip_rect {
    rect_t rect;
    struct clip_rect* next;
} clip_rect_t;

typedef struct region {
    clip_rect_t* rects;
} region_t;

int region_clear( region_t* region );
int region_add( region_t* region, rect_t* rect );
int region_exclude( region_t* region, rect_t* rect );

int region_init( region_t* region );
int region_destroy( region_t* region );

#endif /* _YGUI_REGION_H_ */
