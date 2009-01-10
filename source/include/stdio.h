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

#include <stddef.h>

#define __FILE_CAN_READ    0x01
#define __FILE_CAN_WRITE   0x02
#define __FILE_ERROR       0x04
#define __FILE_EOF         0x08
#define __FILE_BUFLINEWISE 0x10

#define EOF 0xFFFFFFFF

#define _IO_BUFSIZE 2048

#ifndef BUFSIZ
#define BUFSIZ _IO_BUFSIZE
#endif // BUFSIZ

typedef struct FILE {
    int fd;
    int flags;
    char* buffer;
    unsigned int buffer_pos;
    unsigned int buffer_size;
    unsigned int buffer_data_size;
    int has_ungotten;
    int unget_buffer;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

int feof( FILE* stream );
int ferror( FILE* stream );
int fileno( FILE* stream );
int fflush( FILE* stream );

int ungetc( int c, FILE* stream );

int printf( const char* format, ... );
int fprintf( FILE* stream, const char* format, ... );

int sprintf( char* str, const char* format, ... );
int snprintf( char* str, size_t size, const char* format, ... );

int fgetc( FILE* stream );
int getc( FILE* stream );

char* fgets( char* s, int size, FILE* stream );
int fputc( int c, FILE* stream );
int fputs( const char* s, FILE* stream );

#endif // _STDIO_H_
