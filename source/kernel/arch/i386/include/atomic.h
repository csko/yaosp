/* Atomic operations
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _ARCH_ATOMIC_H_
#define _ARCH_ATOMIC_H_

#define ATOMIC_INIT(n) { n }

typedef struct atomic {
    volatile int value;
} atomic_t;

int atomic_get( atomic_t* atomic );
int atomic_set( atomic_t* atomic, int value );
int atomic_inc( atomic_t* atomic );
int atomic_dec( atomic_t* atomic );
int atomic_swap( atomic_t* atomic, int value );

#endif // _ARCH_ATOMIC_H_
