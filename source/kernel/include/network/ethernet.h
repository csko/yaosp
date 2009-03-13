/* Ethernet layer definitions
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

#ifndef _NETWORK_ETHERNET_H_
#define _NETWORK_ETHERNET_H_

#include <types.h>
#include <network/packet.h>

#define ETH_ADDR_LEN   6
#define ETH_HEADER_LEN 14

#define ETH_P_IP  0x0800
#define ETH_P_ARP 0x0806

struct net_interface;

typedef struct ethernet_header {
    uint8_t dest[ ETH_ADDR_LEN ];
    uint8_t src[ ETH_ADDR_LEN ];
    uint16_t proto;
} __attribute__(( packed )) ethernet_header_t;

int ethernet_send_packet( struct net_interface* interface, uint8_t* hw_address, uint16_t protocol, packet_t* packet );

#endif /* _NETWORK_ETHERNET_H_ */
