/* Ethernet layer definitions
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

#ifndef _NETWORK_ETHERNET_H_
#define _NETWORK_ETHERNET_H_

#include <types.h>
#include <macros.h>
#include <network/packet.h>
#include <lib/random.h>

#define ETH_ADDR_LEN   6
#define ETH_HEADER_LEN 14
#define ETH_DATA_LEN   1500
#define ETH_ZLEN       60

#define ETH_P_IP  0x0800
#define ETH_P_ARP 0x0806

struct net_device;

typedef struct ethernet_header {
    uint8_t dest[ ETH_ADDR_LEN ];
    uint8_t src[ ETH_ADDR_LEN ];
    uint16_t proto;
} __PACKED ethernet_header_t;

static inline int is_zero_ethernet_address( uint8_t* address ) {
    return !( address[0] | address[1] | address[2] | address[3] | address[4] | address[5] );
}

static inline int is_multicast_ethernet_address( uint8_t* address ) {
    return ( address[ 0 ] & 0x01 );
}

static inline int is_valid_ethernet_address( uint8_t* address ) {
    return ( ( !is_multicast_ethernet_address( address ) ) &&
             ( !is_zero_ethernet_address( address ) ) );
}

static inline void random_ethernet_address( uint8_t* address ) {
    random_get_bytes( address, ETH_ADDR_LEN );

    address[ 0 ] &= 0xFE; /* clear multicast bit */
    address[ 0 ] |= 0x02; /* set local assignment bit (IEEE802) */
}


int ethernet_send_packet( struct net_device* device, uint8_t* hw_address, uint16_t protocol, packet_t* packet );

#endif /* _NETWORK_ETHERNET_H_ */
