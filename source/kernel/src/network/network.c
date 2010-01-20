/* Commong networking functions
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

#include <config.h>

#ifdef ENABLE_NETWORK

#include <network/interface.h>
#include <network/arp.h>
#include <network/socket.h>
#include <network/tcp.h>
#include <network/udp.h>
#include <network/route.h>

__init void init_network( void ) {
    init_network_interfaces();
    init_routes();
    init_arp();
    init_socket();
    init_tcp();
    init_udp();
}

#endif /* ENABLE_NETWORK */
