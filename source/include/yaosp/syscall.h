/* System calls
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

#ifndef _YAOSP_SYSCALL_H_
#define _YAOSP_SYSCALL_H_

#include <sys/types.h>

int syscall0( int number );
int syscall1( int number, uint32_t p1 );
int syscall2( int number, uint32_t p1, uint32_t p2 );
int syscall3( int number, uint32_t p1, uint32_t p2, uint32_t p3 );
int syscall4( int number, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4 );
int syscall5( int number, uint32_t p1, uint32_t p2, uint32_t p3, uint32_t p4, uint32_t p5 );

#endif // _YAOSP_SYSCALL_H_
