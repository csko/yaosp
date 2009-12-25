/* Bitmap implementation
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

#ifndef _LIB_BITMAP_H_
#define _LIB_BITMAP_H_

#include <types.h>

typedef struct bitmap {
    uint32_t* table;
    int max_bit_count;
} bitmap_t;

int bitmap_get( bitmap_t* bitmap, int bit_index );
int bitmap_set( bitmap_t* bitmap, int bit_index );
int bitmap_unset( bitmap_t* bitmap, int bit_index );

int bitmap_first_not_set( bitmap_t* bitmap );
int bitmap_first_not_set_in_range( bitmap_t* bitmap, int start, int end );

int init_bitmap( bitmap_t* bitmap, int max_bits );
int destroy_bitmap( bitmap_t* bitmap );

#endif /* _LIB_BITMAP_H_ */
