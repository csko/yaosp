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

#include <console.h>
#include <errno.h>
#include <lib/string.h>
#include <vfs/vfs.h>
#include <network/arp.h>
#include <network/ethernet.h>
#include <network/device.h>

static int arp_handle_request( net_interface_t* interface, arp_header_t* arp_header ) {
    int error;
    packet_t* reply;
    arp_data_t* arp_request;
    ethernet_header_t* eth_header;
    arp_header_t* arp_reply_header;
    arp_data_t* arp_reply_data;

    arp_request = ( arp_data_t* )( arp_header + 1 );

    if ( memcmp( arp_request->ip_target, interface->ip_address, IPV4_ADDR_LEN ) != 0 ) {
        return 0;
    }

    /* Build the reply */

    reply = create_packet( ETH_HEADER_LEN + ARP_HEADER_LEN + ARP_DATA_LEN );

    if ( reply == NULL ) {
        kprintf( "NET: No memory for ARP reply packet!\n" );
        return -ENOMEM;
    }

    eth_header = ( ethernet_header_t* )reply->data;
    arp_reply_header = ( arp_header_t* )( eth_header + 1 );
    arp_reply_data = ( arp_data_t* )( arp_reply_header + 1 );

    /* Ethernet header */

    memcpy( eth_header->dest, arp_request->hw_sender, ETH_ADDR_LEN );
    memcpy( eth_header->src, interface->hw_address, ETH_ADDR_LEN );
    eth_header->proto = htonw( ETH_P_ARP );

    /* ARP header */

    arp_reply_header->hardware_addr_format = htonw( ARP_HEADER_ETHER );
    arp_reply_header->protocol_addr_format = htonw( ETH_P_IP );
    arp_reply_header->hardware_addr_size = ETH_ADDR_LEN;
    arp_reply_header->protocol_addr_size = IPV4_ADDR_LEN;
    arp_reply_header->command = htonw( ARP_CMD_REPLY );

    /* ARP data */

    memcpy( arp_reply_data->hw_target, eth_header->dest, ETH_ADDR_LEN );
    memcpy( arp_reply_data->ip_target, arp_request->ip_sender, IPV4_ADDR_LEN );
    memcpy( arp_reply_data->hw_sender, interface->hw_address, ETH_ADDR_LEN );
    memcpy( arp_reply_data->ip_sender, interface->ip_address, IPV4_ADDR_LEN );

    /* Send the packet */

    error = pwrite( interface->device, reply->data, reply->size, 0 );

    delete_packet( reply );

    if ( error < 0 ) {
        kprintf( "NET: Failed to send ARP reply packet: %d\n", error );
    }

    return error;
}

int arp_input( net_interface_t* interface, packet_t* packet ) {
    arp_header_t* arp_header;

    arp_header = ( arp_header_t* )( packet->data + ETH_HEADER_LEN );

    switch ( ntohw( arp_header->command ) ) {
        case ARP_CMD_REQUEST :
            kprintf( "ARP request\n" );
            arp_handle_request( interface, arp_header );
            break;

        case ARP_CMD_REPLY :
            kprintf( "ARP reply\n" );
            break;

        default :
            kprintf( "Unknown ARP command: 0x%x\n", ntohw( arp_header->command ) );
            break;
    }

    return 0;
}
