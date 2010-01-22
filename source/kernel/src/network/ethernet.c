/* Ethernet protocol handling
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <console.h>
#include <network/device.h>
#include <network/ethernet.h>
#include <network/arp.h>

int ethernet_input( net_device_t* device, packet_t* packet ) {
    ethernet_header_t* eth_header;

    eth_header = ( ethernet_header_t* )packet->data;
    packet->network_data = ( uint8_t* )( eth_header + 1 );

    switch ( ntohw( eth_header->proto ) ) {
        case ETH_P_IP :
            ipv4_input( packet );
            break;

        case ETH_P_ARP :
            arp_input( device, packet );
            break;

        default :
            kprintf( WARNING, "net: Unknown protocol: %x\n", ntohw( eth_header->proto ) );
            delete_packet( packet );
            break;
    }

    return 0;
}

int ethernet_send_packet( net_device_t* device, uint8_t* hw_address, uint16_t protocol, packet_t* packet ) {
    int error;
    ethernet_header_t* eth_header;

    eth_header = ( ethernet_header_t* )packet->data;

    /* Build the ethernet header */

    eth_header->proto = htonw( protocol );

    memcpy( eth_header->dest, hw_address, ETH_ADDR_LEN );
    memcpy( eth_header->src, device->dev_addr, ETH_ADDR_LEN );

    /* Send the packet */

    error = net_device_transmit( device, packet );

    if ( error < 0 ) {
        delete_packet( packet );
    }

    return error;
}

#endif /* ENABLE_NETWORK */
