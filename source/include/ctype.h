/* yaosp C library
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#endif // _CTYPE_H_
