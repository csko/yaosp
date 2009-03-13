/* ARP packet handling
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

#ifndef _NETWORK_ARP_H_
#define _NETWORK_ARP_H_

#include <types.h>
#include <lib/hashtable.h>
#include <network/packet.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <network/interface.h>

#define ARP_HEADER_LEN 8
#define ARP_DATA_LEN   ( 2 * ETH_ADDR_LEN + 2 * IPV4_ADDR_LEN )

#define ARP_HEADER_ETHER 1

#define ARP_CMD_REQUEST 1
#define ARP_CMD_REPLY   2

typedef struct arp_header {
    uint16_t hardware_addr_format;
    uint16_t protocol_addr_format;
    uint8_t hardware_addr_size;
    uint8_t protocol_addr_size;
    uint16_t command;
} __attribute__(( packed )) arp_header_t;

typedef struct arp_data {
    uint8_t hw_sender[ ETH_ADDR_LEN ];
    uint8_t ip_sender[ IPV4_ADDR_LEN ];
    uint8_t hw_target[ ETH_ADDR_LEN ];
    uint8_t ip_target[ IPV4_ADDR_LEN ];
} __attribute__(( packed )) arp_data_t;

typedef struct arp_pending_request {
    hashitem_t hash;

    uint8_t ip_address[ IPV4_ADDR_LEN ];
    packet_queue_t* packet_queue;
} arp_pending_request_t;

int arp_send_packet( net_interface_t* interface, uint8_t* dest_ip, packet_t* packet );
int arp_input( net_interface_t* interface, packet_t* packet );

int init_arp( void );

#endif /* _NETWORK_ARP_H_ */
