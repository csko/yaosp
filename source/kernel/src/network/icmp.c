/* ICMP packet handling
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

#include <console.h>
#include <errno.h>
#include <macros.h>
#include <network/icmp.h>
#include <network/ipv4.h>
#include <network/device.h>
#include <network/ethernet.h>
#include <lib/string.h>

static int icmp_handle_echo( packet_t* packet ) {
    uint8_t* data;
    packet_t* reply;
    int echo_payload_size;
    ipv4_header_t* ip_header;
    icmp_header_t* icmp_header;
    icmp_echo_t* echo;
    icmp_echo_reply_t* echo_reply;
    icmp_header_t* icmp_reply_header;
    uint8_t dest_ip[ IPV4_ADDR_LEN ];

    ip_header = ( ipv4_header_t* )packet->network_data;
    icmp_header = ( icmp_header_t* )packet->transport_data;
    echo = ( icmp_echo_t* )( icmp_header + 1 );

    echo_payload_size =
        ntohw( ip_header->packet_size ) -
        IPV4_HDR_SIZE( ip_header->version_and_size ) * 4 -
        sizeof( icmp_echo_t );

    reply = create_packet(
        ETH_HEADER_LEN +
        IPV4_HEADER_LEN +
        ICMP_HEADER_LEN +
        sizeof( icmp_echo_reply_t ) +
        echo_payload_size
    );

    if ( reply == NULL ) {
        kprintf( ERROR, "NET: No memory for ICMP packet!\n" );
        delete_packet( packet );
        return -ENOMEM;
    }

    data = reply->data + reply->size;

    /* Copy ICMP payload */

    if ( echo_payload_size > 0 ) {
        data -= echo_payload_size;

        memcpy(
            data,
            ( void* )( echo + 1 ),
            echo_payload_size
        );
    }

    /* Build echo reply structure */

    data -= sizeof( icmp_echo_reply_t );
    echo_reply = ( icmp_echo_reply_t* )data;

    echo_reply->identifier = echo->identifier;
    echo_reply->sequence = echo->sequence;

    /* Build the ICMP header */

    data -= sizeof( icmp_header_t );
    icmp_reply_header = ( icmp_header_t* )data;
    reply->transport_data = data;

    icmp_reply_header->type = ICMP_ECHO_REPLY;
    icmp_reply_header->code = icmp_header->code;

    memcpy( dest_ip, ip_header->src_address, IPV4_ADDR_LEN );

    /* Now we can delete the original packet */

    delete_packet( packet );

    icmp_reply_header->checksum = 0;
    icmp_reply_header->checksum = ip_checksum(
        ( uint16_t* )icmp_reply_header,
        ICMP_HEADER_LEN + sizeof( icmp_echo_reply_t ) + echo_payload_size
    );

    return ipv4_send_packet( dest_ip, reply, IP_PROTO_ICMP );
}

int icmp_input( packet_t* packet ) {
    icmp_header_t* icmp_header;

    icmp_header = ( icmp_header_t* )packet->transport_data;

    /* TODO: checksum */

    switch ( icmp_header->type ) {
        case ICMP_ECHO :
            icmp_handle_echo( packet );
            break;

        default :
            kprintf( WARNING, "NET: Unknown ICMP type: %x\n", icmp_header->type );
            delete_packet( packet );
            return -EINVAL;
    }

    return 0;
}
