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

#ifndef _YGUI_BORDER_H_
#define _YGUI_BORDER_H_

#include <ygui/gc.h>

struct widget;

typedef struct border_operations {
    int ( *paint )( struct widget* widget, gc_t* gc );
} border_operations_t;

typedef struct border {
    int ref_count;
    point_t size;
    point_t lefttop_offset;
    border_operations_t* ops;
} border_t;

int border_inc_ref( border_t* border );
int border_dec_ref( border_t* border );

border_t* create_border( border_operations_t* ops );

#endif /* _YGUI_BORDER_H_ */
