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

#include <config.h>

#ifdef ENABLE_NETWORK

#include <console.h>
#include <errno.h>
#include <kernel.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <network/arp.h>
#include <network/ethernet.h>
#include <network/device.h>
#include <lib/string.h>

/* 1 sec expire time for ARP cache items */
#define ARP_CACHE_EXPIRE_TIME ( 1 * 1000 * 1000 )

static arp_pending_request_t* arp_create_request( uint8_t* ip_address ) {
    arp_pending_request_t* request;

    request = ( arp_pending_request_t* )kmalloc( sizeof( arp_pending_request_t ) );

    if ( request == NULL ) {
        goto error1;
    }

    packet_queue_init( &request->packet_queue );

    memcpy( request->ip_address, ip_address, IPV4_ADDR_LEN );

    return request;

 error1:
    return NULL;
}

static arp_cache_item_t* arp_cache_create_item( uint8_t* hw_address, uint8_t* ip_address, uint32_t flags ) {
    arp_cache_item_t* item;

    item = ( arp_cache_item_t* )kmalloc( sizeof( arp_cache_item_t ) );

    if ( item == NULL ) {
        return NULL;
    }

    item->flags = flags;
    item->expire_time = get_system_time() + ARP_CACHE_EXPIRE_TIME;

    memcpy( item->hw_address, hw_address, ETH_ADDR_LEN );
    memcpy( item->ip_address, ip_address, IPV4_ADDR_LEN );

    return item;
}

static void* arp_pending_key( hashitem_t* item ) {
    arp_pending_request_t* pending_req;

    pending_req = ( arp_pending_request_t* )item;

    return ( void* )pending_req->ip_address;
}

static void* arp_cache_key( hashitem_t* item ) {
    arp_cache_item_t* cache_item;

    cache_item = ( arp_cache_item_t* )item;

    return ( void* )cache_item->ip_address;
}

int arp_interface_init( net_device_t* device ) {
    int error;
    arp_cache_item_t* item;

    static uint8_t broadcast_hw[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    static uint8_t broadcast_ip[] = { 0xff, 0xff, 0xff, 0xff };

    error = init_hashtable(
        &device->arp_requests, 32,
        arp_pending_key, hash_int, compare_int
    );

    if ( error < 0 ) {
        goto error1;
    }

    error = init_hashtable(
        &device->arp_cache, 256,
        arp_cache_key, hash_int, compare_int
    );

    if ( error < 0 ) {
        goto error2;
    }

    /* Add an ARP entry for the broadcast address */

    item = arp_cache_create_item( broadcast_hw, broadcast_ip, ARP_DONTCLEAN );

    if ( item == NULL ) {
        goto error3;
    }

    hashtable_add( &device->arp_cache, ( hashitem_t* )item );

    return 0;

 error3:
    destroy_hashtable( &device->arp_cache );

 error2:
    destroy_hashtable( &device->arp_requests );

 error1:
    return error;
}

int arp_cache_insert( net_device_t* device, uint8_t* hw_address, uint8_t* ip_address ) {
    arp_cache_item_t* item;
    arp_pending_request_t* pending_req = NULL;

    mutex_lock( device->arp_lock, LOCK_IGNORE_SIGNAL );

    /* Check if we already have this in the cache */

    item = ( arp_cache_item_t* )hashtable_get( &device->arp_cache, ( const void* )ip_address );

    if ( item != NULL ) {
        ASSERT( hashtable_get( &device->arp_requests, ( const void* )ip_address ) == NULL );
        goto out;
    }

    /* Not in the cache, add now */

    item = arp_cache_create_item( hw_address, ip_address, 0 );

    if ( item == NULL ) {
        goto flush;
    }

    if ( hashtable_add( &device->arp_cache, ( hashitem_t* )item ) != 0 ) {
        kfree( item );
    }

    /* Flush any pending packets */

 flush:
    pending_req = ( arp_pending_request_t* )hashtable_get( &device->arp_requests, ( const void* )ip_address );

    if ( pending_req != NULL ) {
        hashtable_remove( &device->arp_requests, ( const void* )ip_address );
    }

 out:
    mutex_unlock( device->arp_lock );

    if ( pending_req != NULL ) {
        packet_t* packet;

        while ( ( packet = packet_queue_pop_head( &pending_req->packet_queue, 0 ) ) != NULL ) {
            ethernet_send_packet( device, hw_address, ETH_P_IP, packet );
        }

        packet_queue_destroy( &pending_req->packet_queue );
        kfree( pending_req );
    }

    return 0;
}

int arp_send_packet( net_device_t* device, uint8_t* dest_ip, packet_t* packet ) {
    bool send_request;
    arp_cache_item_t* item;
    arp_pending_request_t* request;

    static uint8_t broadcast_ip[] = { 0xff, 0xff, 0xff, 0xff };

    if ( ( net_device_flags( device ) & NETDEV_UP ) == 0 ) {
        return -ENETDOWN;
    }

    mutex_lock( device->arp_lock, LOCK_IGNORE_SIGNAL );

    /* First check the ARP cache for the HW address */

    item = ( arp_cache_item_t* )hashtable_get( &device->arp_cache, ( const void* )dest_ip );

    if ( item != NULL ) {
        uint8_t hw_address[ ETH_ADDR_LEN ];

        memcpy( hw_address, item->hw_address, ETH_ADDR_LEN );

        mutex_unlock( device->arp_lock );

        return ethernet_send_packet( device, hw_address, ETH_P_IP, packet );
    }

    ASSERT( !IP_EQUALS( dest_ip, broadcast_ip ) );

    /* Not found in the cache, add to the pending table and send out an ARP request if needed */

    request = ( arp_pending_request_t* )hashtable_get( &device->arp_requests, ( const void* )dest_ip );
    send_request = ( request == NULL );

    if ( request == NULL ) {
        request = arp_create_request( dest_ip );

        if ( request == NULL ) {
            return -ENOMEM;
        }

        hashtable_add( &device->arp_requests, ( hashitem_t* )request );
    }

    packet_queue_insert( &request->packet_queue, packet );

    mutex_unlock( device->arp_lock );

    if ( send_request ) {
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
        memcpy( arp_data->hw_sender, device->dev_addr, ETH_ADDR_LEN );

        /* IP addresses */

        memcpy( arp_data->ip_target, dest_ip, IPV4_ADDR_LEN );
        memcpy( arp_data->ip_sender, device->ip_addr, IPV4_ADDR_LEN );

        return ethernet_send_packet( device, hw_broadcast, ETH_P_ARP, arp_request );
    }

    return 0;
}

static int arp_handle_request( net_device_t* device, arp_header_t* arp_header ) {
    int error;
    packet_t* reply;
    arp_data_t* arp_request;
    ethernet_header_t* eth_header;
    arp_header_t* arp_reply_header;
    arp_data_t* arp_reply_data;

    /* Check if the ARP request is for us */

    arp_request = ( arp_data_t* )( arp_header + 1 );

    if ( !IP_EQUALS( arp_request->ip_target, device->ip_addr ) ) {
        return 0;
    }

    /* Build the reply */

    reply = create_packet( ETH_HEADER_LEN + ARP_HEADER_LEN + ARP_DATA_LEN );

    if ( reply == NULL ) {
        kprintf( ERROR, "net: No memory for ARP reply packet.\n" );
        return -ENOMEM;
    }

    eth_header = ( ethernet_header_t* )reply->data;
    arp_reply_header = ( arp_header_t* )( eth_header + 1 );
    arp_reply_data = ( arp_data_t* )( arp_reply_header + 1 );

    /* Ethernet header */

    memcpy( eth_header->dest, arp_request->hw_sender, ETH_ADDR_LEN );
    memcpy( eth_header->src, device->dev_addr, ETH_ADDR_LEN );
    eth_header->proto = htonw( ETH_P_ARP );

    /* ARP header */

    arp_reply_header->hardware_addr_format = htonw( ARP_HEADER_ETHER );
    arp_reply_header->protocol_addr_format = htonw( ETH_P_IP );
    arp_reply_header->hardware_addr_size = ETH_ADDR_LEN;
    arp_reply_header->protocol_addr_size = IPV4_ADDR_LEN;
    arp_reply_header->command = htonw( ARP_CMD_REPLY );

    /* ARP data */

    memcpy( arp_reply_data->hw_target, eth_header->dest, ETH_ADDR_LEN );
    memcpy( arp_reply_data->hw_sender, device->dev_addr, ETH_ADDR_LEN );
    IP_COPY_ADDR( arp_reply_data->ip_target, arp_request->ip_sender );
    IP_COPY_ADDR( arp_reply_data->ip_sender, device->ip_addr );

    /* Send the packet */

    error = net_device_transmit( device, reply );

    if ( error < 0 ) {
        kprintf( ERROR, "net: failed to send ARP reply packet: %d\n", error );
        delete_packet( reply );
    }

    return 0;
}

int arp_input( net_device_t* device, packet_t* packet ) {
    arp_data_t* arp_data;
    arp_header_t* arp_header;

    arp_header = ( arp_header_t* )( packet->data + ETH_HEADER_LEN );

    switch ( ntohw( arp_header->command ) ) {
        case ARP_CMD_REQUEST :
            arp_data = ( arp_data_t* )( arp_header + 1 );
            arp_cache_insert( device, arp_data->hw_sender, arp_data->ip_sender );

            arp_handle_request( device, arp_header );

            break;

        case ARP_CMD_REPLY :
            arp_data = ( arp_data_t* )( arp_header + 1 );
            arp_cache_insert( device, arp_data->hw_sender, arp_data->ip_sender );

            break;

        default :
            kprintf( WARNING, "net: unknown ARP command: 0x%x\n", ntohw( arp_header->command ) );
            break;
    }

    return 0;
}

__init int init_arp( void ) {
    return 0;
}

#endif /* ENABLE_NETWORK */
