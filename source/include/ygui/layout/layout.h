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

#ifndef _YGUI_LAYOUT_H_
#define _YGUI_LAYOUT_H_

#include <ygui/widget.h>

typedef struct layout_operations {
    int ( *do_layout )( widget_t* widget );
    int ( *get_preferred_size )( widget_t* widget, point_t* size );
} layout_operations_t;

typedef struct layout {
    int ref_count;
    layout_operations_t* ops;
} layout_t;

int layout_inc_ref( layout_t* layout );
int layout_dec_ref( layout_t* layout );

layout_t* create_layout( layout_operations_t* ops );

#endif /* _YAOSP_LAYOUT_H_ */
