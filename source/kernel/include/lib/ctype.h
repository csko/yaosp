/* Ctype macros and functions
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

#ifndef _LIB_CTYPE_H_
#define _LIB_CTYPE_H_

#define isalnum(c) ( isalpha(c) || isdigit(c) )
#define isalpha(c) ( ( 'A' <= c && c <= 'Z' ) || ( 'a' <= c && c <= 'z' ) )
#define isdigit(c) ( '0' <= c && c <= '9' )
#define islower(c) ( 'a' <= c && c <= 'z' )
#define isupper(c) ( 'A' <= c && c <= 'Z' )

int toupper( int c );
int tolower( int c );

#endif // _LIB_CTYPE_H_
