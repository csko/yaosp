/* Terminal driver
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

#ifndef _TERMINAL_TERMINAL_H_
#define _TERMINAL_TERMINAL_H_

#include <types.h>

#define MAX_TERMINAL_COUNT 6

#define TERMINAL_WIDTH 80
#define TERMINAL_HEIGHT 25
#define TERMINAL_MAX_LINES 200

#define TERMINAL_ACCEPTS_USER_INPUT 0x01

#define TERM_BUFFER_LINE_END 0x01

typedef struct term_buffer_item {
    char* buffer;
    size_t size;
    int flags;
} term_buffer_item_t;

typedef struct terminal {
    int master_pty;
    int flags;

    int line_count;
    term_buffer_item_t* lines;
    int start_line;
} terminal_t;

extern terminal_t* terminals[ MAX_TERMINAL_COUNT ];

int terminal_scroll( int offset );
int terminal_switch_to( int index );

#endif // _TERMINAL_TERMINAL_H_
