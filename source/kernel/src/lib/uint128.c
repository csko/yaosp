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

#include <lib/uint128.h>

int uint128_sub( uint128_t* a, uint128_t* b ) {
    uint64_t rl = a->low - b->low;

    a->high = a->high - b->high - ( rl > a->low ? 1 : 0 );
    a->low = rl;

    return 0;
}

int uint128_multiply( uint128_t* i, uint32_t val ) {
    uint64_t rm = ( i->low >> 32 ) * val;
    uint64_t rl = ( i->low & 0xFFFFFFFF ) * val + ( rm << 32 );

    i->low = rl;
    i->high = i->high * val + ( rm >> 32 ) + ( rl < rm << 32 ? 1 : 0 );

    return 0;
}

int uint128_divide( uint128_t* i, uint128_t* d ) {
    int shift = 0;
    uint128_t sd = {
        .low = d->low,
        .high = d->high
    };

	while ( ( ( sd.high >> 63 ) == 0 ) &&
            ( uint128_less( &sd, i ) ) ) {
        uint128_shl( &sd, 1 );
        shift++;
    }

    uint128_t temp = {
        .low = i->low,
        .high = i->high
    };

    i->low = 0;
    i->high = 0;

    for ( ; shift >= 0; shift-- ) {
        if ( uint128_less_or_equals( &sd, &temp ) ) {
            uint128_set_bit( i, shift );
            uint128_sub( &temp, &sd );
        }

        uint128_shr( &sd, 1 );
    }

    return 0;
}

int uint128_shl( uint128_t* i, int count ) {
    if ( count == 0 ) {
        /* do nothing */
    } else if ( count >= 128 ) {
        i->low = 0;
        i->high = 0;
    } else if ( count >= 64 ) {
        i->high = i->low << ( count - 64 );
        i->low = 0;
    } else {
        uint64_t low = i->low;
        i->low <<= count;
        i->high = ( i->high << count ) | ( low >> ( 64 - count ) );
    }

    return 0;
}

int uint128_shr( uint128_t* i, int count ) {
    if ( count == 0 ) {
        /* do nothing */
    } else if ( count >= 128 ) {
        i->low = 0;
        i->high = 0;
    } else if ( count >= 64 ) {
        i->low = ( i->high >> ( count - 64 ) );
        i->high = 0;
    } else {
        uint64_t high = i->high;
        i->high >>= count;
        i->low = ( i->low >> count ) | ( high << ( 64 - count ) );
    }

    return 0;
}

int uint128_set_bit( uint128_t* i, int index ) {
    if ( index >= 128 ) {
        /* do nothing */
    } else if ( index >= 64 ) {
        i->high |= ( 1 << ( index - 64 ) );
    } else if ( index >= 0 ) {
        i->low |= ( 1 << index );
    }

    return 0;
}

int uint128_less( uint128_t* a, uint128_t* b ) {
    return ( ( a->high < b->high ) ||
             ( ( a->high == b->high ) && ( a->low < b->low ) ) );
}

int uint128_less_or_equals( uint128_t* a, uint128_t* b ) {
    return ( ( a->high < b->high ) ||
             ( ( a->high == b->high ) && ( a->low <= b->low ) ) );
}

int uint128_init( uint128_t* i, uint64_t val ) {
    i->low = val;
    i->high = 0;

    return 0;
}
