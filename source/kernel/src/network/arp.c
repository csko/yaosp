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
#include <kernel.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <network/arp.h>
#include <network/ethernet.h>
#include <network/device.h>
#include <lib/string.h>

static hashtable_t pending_requests;

static arp_pending_request_t* arp_create_request( uint8_t* ip_address ) {
    arp_pending_request_t* request;

    request = ( arp_pending_request_t* )kmalloc( sizeof( arp_pending_request_t ) );

    if ( request == NULL ) {
        goto error1;
    }

    request->packet_queue = create_packet_queue();

    if ( request->packet_queue == NULL ) {
        goto error2;
    }

    memcpy( request->ip_address, ip_address, IPV4_ADDR_LEN );

    return request;

error2:
    kfree( request );

error1:
    return NULL;
}

int arp_send_packet( net_interface_t* interface, uint8_t* dest_ip, packet_t* packet ) {
    bool send_request;
    arp_pending_request_t* request;

    request = ( arp_pending_request_t* )hashtable_get( &pending_requests, ( const void* )dest_ip );

    if ( request == NULL ) {
        send_request = true;

        request = arp_create_request( dest_ip );

        if ( request == NULL ) {
            return -ENOMEM;
        }

        hashtable_add( &pending_requests, ( hashitem_t* )request );
    } else {
        send_request = false;
    }

    packet_queue_insert( request->packet_queue, packet );

    if ( send_request ) {
        int error;
        packet_t* arp_request;
        arp_header_t* arp_header;
        arp_data_t* arp_data;
        uint8_t hw_broadcast[ ETH_ADDR_LEN ] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

        arp_request = create_packet( ETH_HEADER_LEN + ARP_HEADER_LEN + ARP_DATA_LEN );

        if ( arp_request == NULL ) {
            return -ENOMEM;
        }

        arp_header = ( arp_header_t* )( arp_request->data + ETH_HEADER_LEN );

        arp_header->hardware_addr_format = htonw( ARP_HEADER_ETHER );
        arp_header->protocol_addr_format = htonw( ETH_P_IP );
        arp_header->hardware_addr_size = ETH_ADDR_LEN;
        arp_header->protocol_addr_size = IPV4_ADDR_LEN;
        arp_header->command = htonw( ARP_CMD_REQUEST );

        arp_data = ( arp_data_t* )( arp_header + 1 );

        /* HW addresses */

        memset( arp_data->hw_target, 0x00, ETH_ADDR_LEN );
        memcpy( arp_data->hw_sender, interface->hw_address, ETH_ADDR_LEN );

        /* IP addresses */

        memcpy( arp_data->ip_target, dest_ip, IPV4_ADDR_LEN );
        memcpy( arp_data->ip_sender, interface->ip_address, IPV4_ADDR_LEN );

        error = ethernet_send_packet( interface, hw_broadcast, ETH_P_ARP, arp_request );

        delete_packet( arp_request );

        if ( error < 0 ) {
            return error;
        }
    }

    return 0;
}

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
        kprintf( ERROR, "NET: No memory for ARP reply packet!\n" );
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
        kprintf( ERROR, "NET: Failed to send ARP reply packet: %d\n", error );
    }

    return error;
}

static int arp_handle_reply( net_interface_t* interface, arp_header_t* arp_header ) {
    packet_t* packet;
    arp_data_t* arp_reply;
    arp_pending_request_t* request;

    arp_reply = ( arp_data_t* )( arp_header + 1 );

    request = ( arp_pending_request_t* )hashtable_get( &pending_requests, ( const void* )&arp_reply->ip_sender[ 0 ] );

    if ( request == NULL ) {
        return 0;
    }

    hashtable_remove( &pending_requests, ( const void* )&request->ip_address[ 0 ] );

    while ( 1 ) {
        packet = packet_queue_pop_head( request->packet_queue, 0 );

        if ( packet == NULL ) {
            break;
        }

        ethernet_send_packet( interface, arp_reply->hw_sender, ETH_P_IP, packet );

        delete_packet( packet );
    }

    delete_packet_queue( request->packet_queue );
    kfree( request );

    return 0;
}

int arp_input( net_interface_t* interface, packet_t* packet ) {
    arp_header_t* arp_header;

    arp_header = ( arp_header_t* )( packet->data + ETH_HEADER_LEN );

    switch ( ntohw( arp_header->command ) ) {
        case ARP_CMD_REQUEST :
            arp_handle_request( interface, arp_header );
            break;

        case ARP_CMD_REPLY :
            arp_handle_reply( interface, arp_header );
            break;

        default :
            kprintf( WARNING, "Unknown ARP command: 0x%x\n", ntohw( arp_header->command ) );
            break;
    }

    return 0;
}

static void* pending_ip_key( hashitem_t* item ) {
    arp_pending_request_t* request;

    request = ( arp_pending_request_t* )item;

    return &request->ip_address[ 0 ];
}

__init int init_arp( void ) {
    int error;

    error = init_hashtable(
        &pending_requests,
        32,
        pending_ip_key,
        hash_int,
        compare_int
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
