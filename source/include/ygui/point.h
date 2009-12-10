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

#ifndef _YGUI_POINT_H_
#define _YGUI_POINT_H_

#include <string.h>
#include <sys/types.h>
#include <sys/param.h>

typedef struct point {
    int x;
    int y;
} point_t;

static inline void point_init( point_t* point, int x, int y ) {
    point->x = x;
    point->y = y;
}

static inline void point_copy( point_t* new, point_t* old ) {
    memcpy( ( void* )new, ( void* )old, sizeof( point_t ) );
}

static inline void point_add( point_t* point1, point_t* point2 ) {
    point1->x += point2->x;
    point1->y += point2->y;
}

static inline void point_add_xy( point_t* point, int x, int y ) {
    point->x += x;
    point->y += y;
}

static inline void point_add_n( point_t* dest, point_t* point1, point_t* point2 ) {
    dest->x = point1->x + point2->x;
    dest->y = point1->y + point2->y;
}

static inline void point_sub( point_t* point1, point_t* point2 ) {
    point1->x -= point2->x;
    point1->y -= point2->y;
}

static inline void point_sub_n( point_t* dest, point_t* src1, point_t* src2 ) {
    dest->x = src1->x - src2->x;
    dest->y = src1->y - src2->y;
}

static inline void point_sub_xy( point_t* point1, int x, int y ) {
    point1->x -= x;
    point1->y -= y;
}

static inline void point_sub_xy_n( point_t* dest, point_t* point, int x, int y ) {
    dest->x = point->x - x;
    dest->y = point->y - y;
}

static inline void point_max( point_t* point1, point_t* point2 ) {
    point1->x = MAX( point1->x, point2->x );
    point1->y = MAX( point1->y, point2->y );
}

static inline void point_max_n( point_t* dest, point_t* point1, point_t* point2 ) {
    dest->x = MAX( point1->x, point2->x );
    dest->y = MAX( point1->y, point2->y );
}

#endif /* _YGUI_POINT_H_ */
