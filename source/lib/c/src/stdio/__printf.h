/* printf implementation
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2008 Kornel Csernai
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

#ifndef __PRINTF_H_
#define __PRINTF_H_

#include <stdarg.h>

#define PRINTF_LEFT     0x01
#define PRINTF_CAPITAL  0x02
#define PRINTF_SIGNED   0x04
#define PRINTF_LONG     0x08
#define PRINTF_SHORT    0x10
#define PRINTF_NEEDSIGN 0x20
#define PRINTF_LZERO    0x40
#define PRINTF_NEEDPLUS 0x80

#define PRINTF_BUFLEN   32

typedef int printf_helper_t( void* data, char c );

int __printf( printf_helper_t* helper, void* data, const char* format, va_list args );

#endif // _PRINTF_H_
