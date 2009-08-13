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
#include <macros.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <network/icmp.h>
#include <network/route.h>
#include <network/arp.h>
#include <network/device.h>
#include <network/tcp.h>
#include <lib/string.h>

#include <arch/network/network.h>

#ifndef ARCH_HAVE_IP_CHECKSUM
uint16_t ip_checksum( uint16_t* data, uint16_t length ) {
    uint32_t checksum;

    checksum = 0;

    while ( length > 1 ) {
        checksum += *data++;

        if ( checksum & 0x80000000 ) {
            checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
        }

        length -= 2;
    }

    if ( length ) {
        checksum += ( uint16_t )( *( ( uint8_t* )data ) );
    }


    while ( checksum >> 16 ) {
        checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
    }

    return ~checksum;
}
#endif /* ARCH_HAVE_IP_CHECKSUM */

int ipv4_send_packet( uint8_t* dest_ip, packet_t* packet, uint8_t protocol ) {
    int error;
    route_t* route;
    ipv4_header_t* ip_header;

    route = find_route( dest_ip );

    if ( route == NULL ) {
        kprintf( WARNING, "NET: No route to address: %d.%d.%d.%d\n", dest_ip[ 0 ], dest_ip[ 1 ], dest_ip[ 2 ], dest_ip[ 3 ] );
        return -EINVAL;
    }

    /* TODO: check MTU */

    ip_header = ( ipv4_header_t* )( packet->transport_data - sizeof( ipv4_header_t ) );

    ASSERT( ( ptr_t )ip_header >= ( ptr_t )packet->data );

    packet->network_data = ( uint8_t* )ip_header;

    ip_header->version_and_size = IPV4_HDR_MK_VER_AND_SIZE( 4, 5 );
    ip_header->type_of_service = 0;
    ip_header->packet_size = htonw( packet->size - ( ( uint32_t )ip_header - ( uint32_t )packet->data ) );
    ip_header->packet_id = 0; /* TODO ??? */
    ip_header->fragment_offset = htonw( IPV4_DONT_FRAGMENT );
    ip_header->time_to_live = 255;
    ip_header->protocol = protocol;

    memcpy( ip_header->src_address, route->interface->ip_address, IPV4_ADDR_LEN );
    memcpy( ip_header->dest_address, dest_ip, IPV4_ADDR_LEN );

    ip_header->checksum = 0;
    ip_header->checksum = ip_checksum( ( uint16_t*)ip_header, sizeof( ipv4_header_t ) );

    if ( route->flags & ROUTE_GATEWAY ) {
        error = arp_send_packet( route->interface, route->gateway_addr, packet );
    } else {
        error = arp_send_packet( route->interface, dest_ip, packet );
    }

    put_route( route );

    return error;
}

static int ipv4_handle_packet( packet_t* packet ) {
    ipv4_header_t* ip_header;

    ip_header = ( ipv4_header_t* )packet->network_data;

    packet->transport_data = ( uint8_t* )ip_header + IPV4_HDR_SIZE( ip_header->version_and_size ) * 4;

    switch ( ip_header->protocol ) {
        case IP_PROTO_TCP :
            return tcp_input( packet );

        case IP_PROTO_UDP :
            DEBUG_LOG( "UDP packet\n" );
            break;

        case IP_PROTO_ICMP :
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
        kprintf( WARNING, "NET: Invalid IP checksum!\n" );
        delete_packet( packet );
        return -EINVAL;
    }

    /* TODO: Handle fragmentation */

    return ipv4_handle_packet( packet );
}
