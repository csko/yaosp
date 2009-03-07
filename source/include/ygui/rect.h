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

#include <sys/types.h>

typedef struct rect {
    uint32_t left;
    uint32_t top;
    uint32_t right;
    uint32_t bottom;
} rect_t;

static inline uint32_t rect_width( rect_t* rect ) {
    return ( rect->right - rect->left + 1 );
}

static inline uint32_t rect_height( rect_t* rect ) {
    return ( rect->bottom - rect->top + 1 );
}

#endif /* _YGUI_RECT_H_ */
