/* Rectangle definition and functions
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

#ifndef _RECT_H_
#define _RECT_H_

typedef struct rect {
    int left;
    int top;
    int right;
    int bottom;
} rect_t;

static inline void rect_bounds( rect_t* rect, int* w, int* h ) {
    *w = rect->right - rect->left + 1;
    *h = rect->bottom - rect->top + 1;
}

#endif /* _RECT_H_ */
