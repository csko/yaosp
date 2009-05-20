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

#ifndef _STDLIB_H_
#define _STDLIB_H_

#define __need_size_t
#define __need_NULL
#include <stddef.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define ATEXIT_MAX 32

#define WEXITSTATUS(status) (((status) & 0xFF00) >> 8)
#define WTERMSIG(status)    ((status) & 0x7F)

#define WIFEXITED(status)   (WTERMSIG(status) == 0)

int abs( int j );
long labs( long j );
long long llabs( long long j );

void exit( int status );
int atexit( void ( *function )( void ) );

char* getenv( const char* name );

void* calloc( size_t nmemb, size_t size ) __attribute__(( malloc ));
void* malloc( size_t size ) __attribute__(( malloc ));
void free( void* ptr );
void* realloc( void* ptr, size_t size );

void abort( void ) __attribute__(( __noreturn__ ));

int atoi( const char* s );
long atol( const char* s );
long long atoll( const char* s );
double atof( const char* s );

long int strtol( const char* nptr, char** endptr, int base );
unsigned long int strtoul( const char* nptr, char** endptr, int base );
double strtod( const char* s, char** endptr );
long int strtol( const char* nptr, char** endptr, int base );
long long int strtoll( const char* nptr, char** endptr, int base );
unsigned long long int strtoull( const char* nptr, char** endptr, int base );

void qsort( void* base, size_t nmemb, size_t size, int ( *compar )( const void*, const void* ) );
void* bsearch( const void* key, const void* base, size_t nmemb, size_t size, int ( *compare )( const void*, const void* ) );

long int random( void );
void srandom( unsigned int seed );
int rand( void );
void srand( unsigned int seed );

char* mktemp( char* template );
int mkstemp( char* template );

#endif /* _STDLIB_H_ */
