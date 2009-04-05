/* yaosp C library
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#ifndef _ARPA_INET_H_
#define _ARPA_INET_H_

#include <netinet/in.h> /* struct in_addr */

int inet_aton( const char* cp, struct in_addr* inp );

in_addr_t inet_addr( const char* cp );

in_addr_t inet_network( const char* cp );

char* inet_ntoa( struct in_addr in );

struct in_addr inet_makeaddr( int net, int host );

in_addr_t inet_lnaof( struct in_addr in );

in_addr_t inet_netof( struct in_addr in );

#endif /* _ARPA_INET_H_ */
