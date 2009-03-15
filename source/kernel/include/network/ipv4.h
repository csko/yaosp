/* IPv4 packet handling
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

#ifndef _NETWORK_IP_H_
#define _NETWORK_IP_H_

#include <types.h>
#include <network/packet.h>

#define IPV4_ADDR_LEN 4
#define IPV4_HEADER_LEN 20

#define IPV4_HDR_VERSION(n)           ((((uint32_t)n)&0xF0)>>4)
#define IPV4_HDR_SIZE(n)              (((uint32_t)n)&0x0F)
#define IPV4_HDR_MK_VER_AND_SIZE(v,s) (((v)<<4)|(s))

#define IPV4_MORE_FRAGMENTS 0x2000
#define IPV4_DONT_FRAGMENT  0x4000
#define IPV4_FRAG_OFF_MASK  0x1FFF

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

typedef struct ipv4_header {
    uint8_t version_and_size;
    uint8_t type_of_service;
    uint16_t packet_size;
    uint16_t packet_id;
    uint16_t fragment_offset;
    uint8_t time_to_live;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t src_address[ IPV4_ADDR_LEN ];
    uint8_t dest_address[ IPV4_ADDR_LEN ];
} __attribute__(( packed )) ipv4_header_t;

uint16_t ip_checksum( uint16_t* data, uint16_t length );

int ipv4_send_packet( uint8_t* dest_ip, packet_t* packet, uint8_t protocol );
int ipv4_input( packet_t* packet );

#endif /* _NETWORK_IP_H_ */
