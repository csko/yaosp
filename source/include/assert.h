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

#undef assert

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) \
    if ( !(expr) ) { __assert_fail( #expr, __FILE__, __LINE__ ); }
#endif /* NDEBUG */

#ifndef _ASSERT_H_
#define _ASSERT_H_

#include <stdio.h>
#include <stdlib.h>
#include <yaosp/debug.h>

static inline void __assert_fail( const char* expr, const char* file, int line ) {
    printf( "Assertion (%s) failed at %s:%d\n", expr, file, line );
    dbprintf( "Assertion (%s) failed at %s:%d\n", expr, file, line );
    abort();
}

#endif /* _ASSERT_H_ */
