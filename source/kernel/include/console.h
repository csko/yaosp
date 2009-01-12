/* Console structure definitions and functions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <types.h>
#include <lib/stdarg.h>

struct console;

typedef void console_init_t( struct console* console );
typedef void console_clear_t( struct console* console );
typedef void console_putchar_t( struct console* console, char c );
typedef void console_gotoxy_t( struct console* console, int x, int y );
typedef void console_flush_t( struct console* console );

typedef struct console_operations {
    console_init_t* init;
    console_clear_t* clear;
    console_putchar_t* putchar;
    console_gotoxy_t* gotoxy;
    console_flush_t* flush;
} console_operations_t;

typedef struct console {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    console_operations_t* ops;
} console_t;

/**
 * This is used by architecture specific part of the kernel to
 * specify the screen properties and callbacks. The arch. code
 * has to fill a console_t structure that is passed to this
 * function as the first argument.
 *
 * @param console The screen instance
 * @return On success 0 is returned
 */
int console_set_screen( console_t* console );
int console_switch_screen( console_t* new_console, console_t** old_console );

int console_set_debug( console_t* console );

int kprintf( const char* format, ... );
int kvprintf( const char* format, va_list args );

int dprintf( const char* format, ... );
int dprintf_unlocked( const char* format, ... );

#endif // _CONSOLE_H_
