/* Route handling
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

#ifndef _NETWORK_ROUTE_H_
#define _NETWORK_ROUTE_H_

#include <types.h>
#include <network/ipv4.h>
#include <network/interface.h>
#include <lib/hashtable.h>

#include <arch/atomic.h>

#define ROUTE_GATEWAY 0x01

typedef struct route {
    hashitem_t hash;

    atomic_t ref_count;
    uint8_t network_addr[ IPV4_ADDR_LEN ];
    uint8_t network_mask[ IPV4_ADDR_LEN ];
    uint8_t gateway_addr[ IPV4_ADDR_LEN ];
    uint32_t flags;
    net_interface_t* interface;
} route_t;

route_t* create_route( uint8_t* net_addr, uint8_t* net_mask, uint8_t* gateway_addr, uint32_t flags );
int insert_route( route_t* route );

route_t* find_route( uint8_t* ipv4_address );
void put_route( route_t* route );

int init_routes( void );

#endif /* _NETWORK_ROUTE_H_ */
