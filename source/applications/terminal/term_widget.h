/* Terminal application
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

#ifndef _TERM_WIDGET_H_
#define _TERM_WIDGET_H_

#include <ygui/widget.h>
#include <ygui/font.h>

#include "terminal.h"

typedef struct terminal_widget {
    font_t* font;
    terminal_t* terminal;
} terminal_widget_t;

int terminal_widget_get_character_size( widget_t* widget, int* width, int* height );

widget_t* create_terminal_widget( terminal_t* terminal );

#endif /* _TERM_WIDGET_H_ */
