/* inet_ntoa function
 *
 * Copyright (c) 2009 Kornel Csernai
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
#include <stdio.h>

static char __inet_ntoa_result[ 18 ];

char* inet_ntoa( struct in_addr in ) {
    uint8_t* bytes;

    bytes = ( uint8_t* )&in;

    snprintf(
        __inet_ntoa_result, sizeof( __inet_ntoa_result ), "%d.%d.%d.%d",
        bytes[ 0 ], bytes[ 1 ], bytes[ 2 ], bytes[ 3 ]
    );

    return __inet_ntoa_result;
}
