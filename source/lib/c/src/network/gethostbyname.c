/* gethostbyname function
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

#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <arpa/inet.h>
#include <yaosp/debug.h>

#include "dns.h"

#define MAX_ADDR_ENTRY 8

static struct in_addr addr4[MAX_ADDR_ENTRY];
static struct in6_addr addr6[MAX_ADDR_ENTRY];
static char* addr_list[MAX_ADDR_ENTRY + 1];

static struct hostent hent;

static int build_hostent_result_v4( const char* name, size_t cnt ) {
    size_t i;

    for ( i = 0; i < cnt; i++ ) {
        addr_list[i] = (char*)&addr4[i];
    }
    addr_list[cnt] = NULL;

    hent.h_name = (char*)name;
    hent.h_aliases = NULL;
    hent.h_addrtype = AF_INET;
    hent.h_length = 4;
    hent.h_addr_list = addr_list;

    return 0;
}

struct hostent* gethostbyname( const char* name ) {
    dbprintf( "gethostbyname(): name = '%s'\n", name );

    if ( inet_pton( AF_INET, name, &addr4[0] ) == 1 ) {
        build_hostent_result_v4(name, 1);
    } else if ( inet_pton( AF_INET6, name, &addr6[0] ) == 1 ) {
        /* TODO: IPv6 */
    } else {
        size_t v4_count;
        struct in_addr* v4_table;

        if ( dns_resolv( (char*)name, &v4_table, &v4_count ) != 0 ) {
            return NULL;
        }

        memcpy( addr4, v4_table, sizeof(struct in_addr) * MIN(v4_count, MAX_ADDR_ENTRY) );
        free( v4_table );

        build_hostent_result_v4( name, v4_count );
    }

    return &hent;
}
