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

char* inet_ntoa( struct in_addr in ) {
    unsigned int ip;
    static char __inet_ntoa_result[18];
    int i;
    uint8_t bytes[4];
    uint8_t* addrbyte;

    ip = in.s_addr;

    addrbyte = (uint8_t *)&ip;

    for(i = 0; i < 4; i++) {
        bytes[i] = *addrbyte++;
    }

    snprintf (__inet_ntoa_result, 18, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);

    return __inet_ntoa_result;
}
