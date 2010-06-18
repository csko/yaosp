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

#ifndef _LINE_H_
#define _LINE_H_

typedef enum terminal_color {
    T_COLOR_BLACK,
    T_COLOR_RED,
    T_COLOR_GREEN,
    T_COLOR_YELLOW,
    T_COLOR_BLUE,
    T_COLOR_MAGENTA,
    T_COLOR_CYAN,
    T_COLOR_WHITE,
    T_COLOR_COUNT
} terminal_color_t;

typedef struct terminal_attr {
    terminal_color_t bg_color;
    terminal_color_t fg_color;
} terminal_attr_t;

typedef struct terminal_line {
    int size;
    char* buffer;
    terminal_attr_t* attr;
} terminal_line_t;

#endif /* _LINE_H_ */
