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

#include <types.h>
#include <console.h>
#include <errno.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <network/icmp.h>

#ifndef ARCH_HAVE_IP_CHECKSUM
static uint16_t ip_checksum( uint16_t* data, uint16_t length ) {
    uint32_t checksum;

    checksum = 0;

    while ( length > 1 ) {
        checksum += *data++;

        if ( checksum & 0x80000000 ) {
            checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
        }

        length -= 2;
    }

    while ( checksum >> 16 ) {
        checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
    }

    return ~checksum;
}
#endif /* ARCH_HAVE_IP_CHECKSUM */

static int ipv4_handle_packet( packet_t* packet ) {
    ipv4_header_t* ip_header;

    ip_header = ( ipv4_header_t* )( packet->data + ETH_HEADER_LEN );

    packet->transport_data = ( uint8_t* )ip_header + IPV4_HDR_SIZE( ip_header->version_and_size ) * 4;

    switch ( ip_header->protocol ) {
        case IP_PROTO_TCP :
            kprintf( "TCP packet\n" );
            break;

        case IP_PROTO_UDP :
            kprintf( "UDP packet\n" );
            break;

        case IP_PROTO_ICMP :
            kprintf( "ICMP packet\n" );
            return icmp_input( packet );
    }

    delete_packet( packet );

    return 0;
}

int ipv4_input( packet_t* packet ) {
    ipv4_header_t* ip_header;

    ip_header = ( ipv4_header_t* )( packet->data + ETH_HEADER_LEN );

    /* Make sure the IP packet is valid */

    if ( ip_checksum( ( uint16_t* )ip_header, IPV4_HDR_SIZE( ip_header->version_and_size ) * 4 ) != 0 ) {
        kprintf( "NET: Invalid IP checksum!\n" );
        delete_packet( packet );
        return -EINVAL;
    }

    /* TODO: Handle fragmentation */

    return ipv4_handle_packet( packet );
}
