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

#ifndef _HISTORY_H_
#define _HISTORY_H_

#include <yutil/array.h>

#include "line.h"

typedef struct terminal_history {
    int width;
    array_t lines;
} terminal_history_t;

int terminal_history_add_line( terminal_history_t* history, terminal_line_t* line );

int terminal_history_get_size( terminal_history_t* history );
terminal_line_t* terminal_history_get_line_at( terminal_history_t* history, int index );

int terminal_history_init( terminal_history_t* history, int width );

#endif /* _HISTORY_H_ */
