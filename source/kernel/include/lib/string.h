/* Memory and string manipulator functions
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

#ifndef _LIB_STRING_H_
#define _LIB_STRING_H_

#include <types.h>

void* memset( void* s, int c, size_t n );
void* memsetw( void* s, int c, size_t n );
void* memsetl( void* s, int c, size_t n );

void* memcpy( void* d, const void* s, size_t n );
void* memmove( void* dest, const void* src, size_t n );
int memcmp( const void* s1, const void* s2, size_t n );

size_t strlen( const char* s );
int strcmp( const char* s1, const char* s2 );
int strncmp( const char* s1, const char* s2, size_t c );
int strcasecmp( const char* s1, const char* s2 );
int strncasecmp( const char* s1, const char* s2, size_t c );
char* strchr( const char* s, int c );
char* strrchr( const char* s, int c );
char* strncpy( char* d, const char* s, size_t c );
char* strdup( const char* s );
char* strndup( const char* s, size_t length );
char* strcat(char *dest, const char *src);
char* strncat(char *dest, const char *src, size_t n);

int snprintf( char* str, size_t size, const char* format, ... );

bool str_to_num( const char* string, int* _number );

#endif /* _LIB_STRING_H_ */
