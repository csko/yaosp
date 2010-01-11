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

#ifndef _BUFFER_H_
#define _BUFFER_H_

#include "history.h"

typedef struct terminal_buffer {
    int width;
    int height;

    int cursor_x;
    int cursor_y;
    int saved_cursor_x;
    int saved_cursor_y;

    int scroll_top;
    int scroll_bottom;

    terminal_attr_t attr;
    terminal_attr_t saved_attr;

    terminal_line_t* lines;
    terminal_history_t history;
} terminal_buffer_t;

#define terminal_attr_compare(a1,a2) memcmp((void*)(a1),(void*)(a2),sizeof(terminal_attr_t))
#define terminal_attr_copy(a1,a2)    memcpy((void*)(a1),(void*)(a2),sizeof(terminal_attr_t))

/* attributes */

int terminal_buffer_set_bg( terminal_buffer_t* buffer, terminal_color_t bg );
int terminal_buffer_set_fg( terminal_buffer_t* buffer, terminal_color_t fg );
int terminal_buffer_swap_bgfg( terminal_buffer_t* buffer );
int terminal_buffer_save_attr( terminal_buffer_t* buffer );
int terminal_buffer_restore_attr( terminal_buffer_t* buffer );

/* insert */

int terminal_buffer_insert_cr( terminal_buffer_t* buffer );
int terminal_buffer_insert_lf( terminal_buffer_t* buffer );
int terminal_buffer_insert_backspace( terminal_buffer_t* buffer );
int terminal_buffer_insert_space( terminal_buffer_t* buffer, int count );
int terminal_buffer_insert_char( terminal_buffer_t* buffer, char c );

/* erase */

int terminal_buffer_erase_above( terminal_buffer_t* buffer );
int terminal_buffer_erase_below( terminal_buffer_t* buffer );
int terminal_buffer_erase_before( terminal_buffer_t* buffer );
int terminal_buffer_erase_after( terminal_buffer_t* buffer );
int terminal_buffer_erase( terminal_buffer_t* buffer, int count );
int terminal_buffer_delete( terminal_buffer_t* buffer, int count );

/* cursor */

int terminal_buffer_save_cursor( terminal_buffer_t* buffer );
int terminal_buffer_restore_cursor( terminal_buffer_t* buffer );
int terminal_buffer_move_cursor( terminal_buffer_t* buffer, int dx, int dy );
int terminal_buffer_move_cursor_to( terminal_buffer_t* buffer, int x, int y );

/* scroll */

int terminal_buffer_scroll_by( terminal_buffer_t* buffer, int lines );
int terminal_buffer_set_scroll_region( terminal_buffer_t* buffer, int top, int bottom );

/* misc */

terminal_line_t* terminal_buffer_get_line_at( terminal_buffer_t* buffer, int index );
int terminal_buffer_get_line_count( terminal_buffer_t* buffer );
int terminal_buffer_get_history_size( terminal_buffer_t* buffer );
int terminal_buffer_dump( terminal_buffer_t* buffer );

int terminal_buffer_init( terminal_buffer_t* buffer, int width, int height );

#endif /* _BUFFER_H_ */
