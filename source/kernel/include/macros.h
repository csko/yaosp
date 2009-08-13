/* Common macro definitions
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#ifndef _MACROS_H_
#define _MACROS_H_

#include <kernel.h>
#include <config.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)<(b)?(b):(a))

#define ALIGN(n,a) (((n)+(a)-1)/(a)*(a))

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

#define ROUND_UP(n,a) (((n)+(a)-1) & ~((a)-1))
#define ROUND_DOWN(n,a) ((n) & ~((a)-1))

#if __GNUC__ < 3
#define __expect(foo,bar) (foo)
#else
#define __expect(foo,bar) __builtin_expect((long)(foo),bar)
#endif

#define __likely(foo) __expect((foo),1)
#define __unlikely(foo) __expect((foo),0)

#ifdef MK_RELEASE_BUILD
#define ASSERT(exp)
#else
#define ASSERT(exp) \
    if ( __unlikely( !( exp ) ) ) { \
        panic( "Assertion (%s) failed at: %s:%d\n", \
            #exp, __FILE__, __LINE__ \
        ); \
    }
#endif /* MK_RELEASE_BUILD */

#endif /* _MACROS_H_ */
