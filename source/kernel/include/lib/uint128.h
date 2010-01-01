/* 128 bit unsigned integer implementation
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#ifndef _LIB_UINT128_H_
#define _LIB_UINT128_H_

#include <types.h>

typedef struct uint128 {
    uint64_t low;
    uint64_t high;
} uint128_t;

int uint128_sub( uint128_t* a, uint128_t* b );
int uint128_multiply( uint128_t* i, uint32_t val );
int uint128_divide( uint128_t* i, uint128_t* d );
int uint128_shl( uint128_t* i, int count );
int uint128_shr( uint128_t* i, int count );
int uint128_set_bit( uint128_t* i, int index );

int uint128_less( uint128_t* a, uint128_t* b );
int uint128_less_or_equals( uint128_t* a, uint128_t* b );

int uint128_init( uint128_t* i, uint64_t val );

#endif /* _LIB_UINT128_H_ */
