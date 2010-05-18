/* yaosp C library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#ifndef _CTYPE_H_
#define _CTYPE_H_

#define _U 1
#define _L 2
#define _N 4
#define _X 8
#define _S 16
#define _P 32
#define _B 64
#define _C 128

#ifdef __cplusplus
extern "C" {
#endif

extern char* _ctype_;

int isupper( int c );
int islower( int c );
int isalpha( int c );
int isdigit( int c );
int isxdigit( int c );
int isalnum( int c );
int isblank( int c );
int isspace( int c );
int isprint( int c );
int iscntrl( int c );
int isgraph( int c );
int ispunct( int c );
int isascii( int c );

int tolower( int c );
int toupper( int c );

#ifdef __cplusplus
}
#endif

#endif /* _CTYPE_H_ */
