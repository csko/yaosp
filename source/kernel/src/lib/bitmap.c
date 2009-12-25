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

#include <errno.h>
#include <mm/kmalloc.h>
#include <lib/bitmap.h>

int bitmap_get( bitmap_t* bitmap, int bit_index ) {
    if ( ( bit_index < 0 ) ||
         ( bit_index >= bitmap->max_bit_count ) ) {
        return 0;
    }

    int idx1 = bit_index / 32;
    int idx2 = bit_index % 32;

    return ( bitmap->table[ idx1 ] & ( 1 << idx2 ) );
}

int bitmap_set( bitmap_t* bitmap, int bit_index ) {
    if ( ( bit_index < 0 ) ||
         ( bit_index >= bitmap->max_bit_count ) ) {
        return -EINVAL;
    }

    int idx1 = bit_index / 32;
    int idx2 = bit_index % 32;

    bitmap->table[ idx1 ] |= ( 1 << idx2 );

    return 0;
}

int bitmap_unset( bitmap_t* bitmap, int bit_index ) {
    if ( ( bit_index < 0 ) ||
         ( bit_index >= bitmap->max_bit_count ) ) {
        return -EINVAL;
    }

    int idx1 = bit_index / 32;
    int idx2 = bit_index % 32;

    bitmap->table[ idx1 ] &= ~( 1 << idx2 );

    return 0;
}

int bitmap_first_not_set( bitmap_t* bitmap ) {
    return bitmap_first_not_set_in_range( bitmap, 0, bitmap->max_bit_count - 1 );
}

int bitmap_first_not_set_in_range( bitmap_t* bitmap, int start, int end ) {
    int i;

    for ( i = start; i <= end; i++ ) {
        int idx1 = i / 32;
        int idx2 = i % 32;

        if ( ( bitmap->table[ idx1 ] & ( 1 << idx2 ) ) == 0 ) {
            return i;
        }
    }

    return -1;
}

int init_bitmap( bitmap_t* bitmap, int max_bits ) {
    bitmap->table = ( uint32_t* )kmalloc( sizeof( uint32_t ) * ( ( max_bits + 31 ) & ~31 ) );

    if ( bitmap->table == NULL ) {
        return -ENOMEM;
    }

    bitmap->max_bit_count = max_bits;

    return 0;
}

int destroy_bitmap( bitmap_t* bitmap ) {
    kfree( bitmap->table );
    bitmap->table = NULL;

    return 0;
}
