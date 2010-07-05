/* inet_pton function
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <yaosp/debug.h>

static int inet_pton4( const char* src, uint8_t* dst ) {
    int ch;
    int octets;
    int saw_digit;
    uint8_t* tp;
    uint8_t tmp[ 4 ];

    saw_digit = 0;
    octets = 0;
    *(tp = tmp) = 0;

    while ( ( ch = *src++ ) != 0 ) {
        if ( isdigit(ch) ) {
            unsigned int new = *tp * 10 + (ch - '0');

            if (new > 255) {
                return 0;
            }

            *tp = new;

            if ( !saw_digit ) {
                if ( ++octets > 4 ) {
                    return 0;
                }

                saw_digit = 1;
            }
        } else if ( ( ch == '.' ) &&
                    ( saw_digit ) ) {
            if ( octets == 4 ) {
                return 0;
            }

            *++tp = 0;
            saw_digit = 0;
        } else {
            return 0;
        }
    }

    if (octets < 4) {
        return 0;
    }

    memcpy( dst, tmp, 4 );

    return 1;
}

int inet_pton( int af, const char* src, void* dst ) {
    switch ( af ) {
        case AF_INET :
            return inet_pton4( src, (uint8_t*)dst );

        /* todo: add IPv6 support */

        default :
            dbprintf( "TODO: IPv6 support for inet_pton not yet implemented!\n" );
            errno = EAFNOSUPPORT;
            return -1;
    }
}
