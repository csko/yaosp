/* printf() like function for the kernel
 *
 * Copyright (c) 2008 Zoltan Kovacs, Kornel Csernai
 * Copyright (c) 2009 Kornel Csernai
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

#ifndef _PRINTF_H_
#define _PRINTF_H_

#define PRINTF_LEFT     0x01
#define PRINTF_CAPITAL  0x02
#define PRINTF_SIGNED   0x04
#define PRINTF_LONG     0x08
#define PRINTF_SHORT    0x10
#define PRINTF_NEEDSIGN 0x20
#define PRINTF_LZERO    0x40
#define PRINTF_NEEDPLUS 0x80
#define PRINTF_LONGLONG 0x100

#define PRINTF_BUFLEN   32

#include <lib/stdarg.h>

typedef int printf_helper_t( void* data, char c );

int do_printf( printf_helper_t* helper, void* data, const char* format, va_list args );

#endif // _PRINTF_H_
