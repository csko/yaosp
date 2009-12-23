/* inet_ntop function
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

#include <stdio.h>
#include <arpa/inet.h>

const char* inet_ntop( int af, const void* src, char* dst, socklen_t size ) {
    const uint8_t* data;

    data = ( uint8_t* )src;

    switch ( af ) {
        case AF_INET :
            snprintf( dst, size, "%d.%d.%d.%d", data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ] );
            break;

        default :
            return NULL;
    }

    return dst;
}
