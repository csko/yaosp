/* yaosp C library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#ifndef __THROW
# ifndef __GNUC_PREREQ
#  define __GNUC_PREREQ(maj, min) (0)
# endif
# if defined __cplusplus && __GNUC_PREREQ (2,8)
#  define __THROW   throw ()
# else
#  define __THROW
# endif
#endif

#define __need_size_t
#define __need_NULL
#include <stddef.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

#define ATEXIT_MAX 32

#define WEXITSTATUS(status) (((status) & 0xFF00) >> 8)
#define WTERMSIG(status)    ((status) & 0x7F)

#define WIFEXITED(status)   (WTERMSIG(status) == 0)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int quot;
    int rem;
} div_t;

typedef struct {
    long int quot;
    long int rem;
} ldiv_t;

int abs( int j );
long labs( long j );
long long llabs( long long j );

div_t div( int num, int denom );
ldiv_t ldiv( long num, long denom );

int system( const char* command );

void exit( int status ) __THROW __attribute__(( __noreturn__ ));
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

char* mktemp( char* tmpl );
int mkstemp( char* tmpl );

#ifdef __cplusplus
}
#endif

#endif /* _STDLIB_H_ */
