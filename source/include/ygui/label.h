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

#ifndef _YGUI_LABEL_H_
#define _YGUI_LABEL_H_

#include <ygui/widget.h>

widget_t* create_label( const char* text );

int label_set_text( widget_t* widget, const char* text );
int label_set_vertical_alignment( widget_t* widget, v_alignment_t alignment );
int label_set_horizontal_alignment( widget_t* widget, h_alignment_t alignment );

#endif /* _YGUI_LABEL_H_ */
