/* Random number generator
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

#include <lib/random.h>

static uint32_t _seed = 0xDEADBEEF;

static uint32_t get_random_number( void ) {
    uint32_t next = _seed;
    uint32_t result;

    next *= 1103515245;
    next += 12345;
    result = ( uint32_t )( next / 65536 ) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( uint32_t )( next / 65536 ) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( uint32_t )( next / 65536 ) % 1024;

    _seed = next;

    return result;
}

int random_get_bytes( uint8_t* data, size_t size ) {
    size_t i;

    for ( i = 0; i < size; i++ ) {
        data[ i ] = get_random_number() % 256;
    }

    return 0;
}

int random_init( uint32_t seed ) {
    _seed = seed;

    return 0;
}
