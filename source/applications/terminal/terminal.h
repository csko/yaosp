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

#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#include <sys/types.h>

#include <yaosp/semaphore.h>

#define TERMINAL_MAX_PARAMS 10

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

typedef enum terminal_state {
    STATE_NONE,
    STATE_ESCAPE,
    STATE_SQUARE_BRACKET,
    STATE_QUESTION
} terminal_state_t;

typedef struct terminal_color_item {
    terminal_color_t bg_color;
    terminal_color_t fg_color;
} terminal_color_item_t;

typedef struct terminal_line {
    int size;
    int max_size;
    char* buffer;
    terminal_color_item_t* buffer_color;
} terminal_line_t;

typedef struct terminal {
    semaphore_id lock;

    int width;
    int height;

    int cursor_x;
    int cursor_y;
    terminal_color_t bg_color;
    terminal_color_t fg_color;

    int first_number;
    int parameter_count;
    int parameters[ TERMINAL_MAX_PARAMS ];
    terminal_state_t state;

    int max_lines;
    int last_line;
    terminal_line_t* lines;
} terminal_t;

int terminal_handle_data( terminal_t* terminal, uint8_t* data, int size );

terminal_t* create_terminal( int width, int height );
int destroy_terminal( terminal_t* terminal );

#endif /* _TERMINAL_H_ */
