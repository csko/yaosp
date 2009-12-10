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

#ifndef _YGUI_RECT_H_
#define _YGUI_RECT_H_

#include <string.h>
#include <sys/types.h>
#include <sys/param.h>

#include <ygui/point.h>

typedef struct rect {
    int left;
    int top;
    int right;
    int bottom;
} rect_t;

static inline void rect_init( rect_t* rect, int left, int top, int right, int bottom ) {
    rect->left = left;
    rect->top = top;
    rect->right = right;
    rect->bottom = bottom;
}

static inline void rect_copy( rect_t* new, rect_t* old ) {
    memcpy( ( void* )new, ( void* )old, sizeof( rect_t ) );
}

static inline int rect_width( rect_t* rect ) {
    return ( rect->right - rect->left + 1 );
}

static inline int rect_height( rect_t* rect ) {
    return ( rect->bottom - rect->top + 1 );
}

static inline void rect_bounds( rect_t* rect, int* w, int* h ) {
    *w = rect->right - rect->left + 1;
    *h = rect->bottom - rect->top + 1;
}

static inline void rect_lefttop( rect_t* rect, point_t* lefttop ) {
    lefttop->x = rect->left;
    lefttop->y = rect->top;
}

static inline void rect_add_point( rect_t* rect, point_t* point ) {
    rect->left = rect->left + point->x;
    rect->right = rect->right + point->x;
    rect->top = rect->top + point->y;
    rect->bottom = rect->bottom + point->y;
}

static inline void rect_add_point_n( rect_t* dst, rect_t* src1, point_t* src2 ) {
    dst->left = src1->left + src2->x;
    dst->right = src1->right + src2->x;
    dst->top = src1->top + src2->y;
    dst->bottom = src1->bottom + src2->y;
}

static inline void rect_add_point_xy( rect_t* rect, int x, int y ) {
    rect->left += x;
    rect->top += y;
    rect->right += x;
    rect->bottom += y;
}

static inline void rect_sub_point( rect_t* rect, point_t* point ) {
    rect->left -= point->x;
    rect->right -= point->x;
    rect->top -= point->y;
    rect->bottom -= point->y;
}

static inline void rect_sub_point_n( rect_t* dest, rect_t* rect, point_t* point ) {
    dest->left = rect->left - point->x;
    dest->right = rect->right - point->x;
    dest->top = rect->top - point->y;
    dest->bottom = rect->bottom - point->y;
}

static inline void rect_sub_point_xy( rect_t* rect, int x, int y ) {
    rect->left -= x;
    rect->right -= x;
    rect->top -= y;
    rect->bottom -= y;
}

static inline void rect_and( rect_t* rect1, rect_t* rect2 ) {
    rect1->left = MAX( rect1->left, rect2->left );
    rect1->right = MIN( rect1->right, rect2->right );
    rect1->top = MAX( rect1->top, rect2->top );
    rect1->bottom = MIN( rect1->bottom, rect2->bottom );
}

static inline void rect_and_n( rect_t* dst, rect_t* rect1, rect_t* rect2 ) {
    dst->left = MAX( rect1->left, rect2->left );
    dst->right = MIN( rect1->right, rect2->right );
    dst->top = MAX( rect1->top, rect2->top );
    dst->bottom = MIN( rect1->bottom, rect2->bottom );
}

static inline int rect_is_valid( rect_t* rect ) {
    return ( ( rect->left <= rect->right ) &&
             ( rect->top <= rect->bottom ) );
}

static inline int rect_has_point( rect_t* rect, point_t* point ) {
    return ( ( rect->left <= point->x ) && ( point->x <= rect->right ) &&
             ( rect->top <= point->y ) && ( point->y <= rect->bottom ) );
}

static inline int rect_has_point_xy( rect_t* rect, int x, int y ) {
    return ( ( rect->left <= x ) && ( x <= rect->right ) &&
             ( rect->top <= y ) && ( y <= rect->bottom ) );
}

static inline int rect_has_rect( rect_t* rect1, rect_t* rect2 ) {
    return ( rect_has_point_xy( rect1, rect2->left, rect2->top ) &&
             rect_has_point_xy( rect1, rect2->right, rect2->bottom ) );
}

static inline int rect_is_equal( rect_t* rect1, rect_t* rect2 ) {
    return ( memcmp( rect1, rect2, sizeof( rect_t ) ) == 0 );
}

static inline void rect_resize( rect_t* rect, int l, int t, int r, int b ) {
    rect->left += l;
    rect->top += t;
    rect->right += r;
    rect->bottom += b;
}

static inline void rect_resize_n( rect_t* dest, rect_t* rect, int l, int t, int r, int b ) {
    dest->left = rect->left + l;
    dest->top = rect->top + t;
    dest->right = rect->right + r;
    dest->bottom = rect->bottom + b;
}

#endif /* _YGUI_RECT_H_ */
