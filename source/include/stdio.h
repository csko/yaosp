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

#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdio.h>

#define FILE_CAN_READ  0x01
#define FILE_CAN_WRITE 0x02

typedef struct FILE {
    int fd;
    int flags;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

int fgetc( FILE* stream );
int fputc( int c, FILE* stream );
int fputs( const char* s, FILE* stream );

#endif // _STDIO_H_
