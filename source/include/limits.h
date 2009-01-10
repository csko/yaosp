/* yaosp C library
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

#ifndef _LIMITS_H_
#define _LIMITS_H_

#ifndef __INT_MAX__
#define __INT_MAX__ 2147483647
#endif
#ifndef __LONG_MAX__
#if __WORDSIZE == 64
#define __LONG_MAX__ 9223372036854775807L
#else
#define __LONG_MAX__ 2147483647L
#endif
#endif

#define CHAR_BIT 8

#define SCHAR_MIN     (-128)
#define SCHAR_MAX     127

#define UCHAR_MAX     255

#ifdef __CHAR_UNSIGNED__
#define CHAR_MIN     0
#define CHAR_MAX     UCHAR_MAX
#else
#define CHAR_MIN     SCHAR_MIN
#define CHAR_MAX     SCHAR_MAX
#endif

#define INT_MIN         (-1 - INT_MAX)
#define INT_MAX         (__INT_MAX__)
#define UINT_MAX        (INT_MAX * 2U + 1U)

#define LONG_MIN        (-1L - LONG_MAX)
#define LONG_MAX        ((__LONG_MAX__) + 0L)
#define ULONG_MAX       (LONG_MAX * 2UL + 1UL)

#endif // _LIMITS_H_
