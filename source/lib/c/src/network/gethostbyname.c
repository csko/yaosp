/* accept function
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

#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <yaosp/debug.h>

static char addr[ 4 ];
static char* addrs[ 2 ];

static struct hostent hent;

struct hostent* gethostbyname( const char* name ) {
    dbprintf( "gethostbyname(): name = %s\n", name );

    hent.h_name = name;
    hent.h_aliases = NULL;
    hent.h_addrtype = AF_INET;
    hent.h_length = 4;
    hent.h_addr_list = addrs;

    addrs[ 0 ] = addr;
    addrs[ 1 ] = NULL;

    if ( inet_pton( AF_INET, name, &addr ) != 1 ) {
        return NULL;
    }

    return &hent;
}
