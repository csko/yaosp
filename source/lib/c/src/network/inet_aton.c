/* inet_aton function
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

#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int inet_aton( const char* cp, struct in_addr* inp ) {
    int i;
    unsigned int ip = 0;
    char* tmp= ( char* )cp;

    for ( i = 24; ; ) {
        long j;

        j = strtoul( tmp, &tmp, 0 );

        if ( *tmp == 0 ) {
            ip |= j;

            break;
        } else if ( *tmp == '.' ) {
            if ( j > 255 ) {
                return 0;
            }

            ip |= ( j << i );

            if ( i > 0 ) {
                i -= 8;
            }

            ++tmp;

            continue;
        }

        return 0;
    }

    inp->s_addr = htonl( ip );

    return 1;
}
