/* Network device definitions
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

#ifndef _NETWORK_DEVICE_H_
#define _NETWORK_DEVICE_H_

#include <types.h>
#include <thread.h>
#include <network/packet.h>
#include <network/interface.h>
#include <network/ipv4.h>
#include <lib/hashtable.h>

#define ntohw(n) \
    ((((uint16_t)(n) & 0xFF) << 8) | ((uint16_t)(n) >> 8))
#define ntohl(n) \
    (((uint32_t)(n) << 24) | (((uint32_t)(n) & 0xFF00) << 8) | (((uint32_t)(n) & 0x00FF0000) >> 8) | ((uint32_t)(n) >> 24))

#define htonw ntohw
#define htonl ntohl

enum {
    NETDEV_CARRIER_ON = ( 1 << 0 ),
    NETDEV_RUNNING = ( 1 << 1 ),
    NETDEV_UP = ( 1 << 2 )
};

typedef enum netdev_tx {
    NETDEV_TX_OK = 0,
    NETDEV_TX_BUSY
} netdev_tx_t;

typedef struct net_device_stats {
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
} net_device_stats_t;

struct net_device;

typedef struct net_device_ops {
    int ( *open )( struct net_device* dev );
    int ( *close )( struct net_device* dev );
    netdev_tx_t ( *transmit )( struct net_device* dev, packet_t* packet );
} net_device_ops_t;

typedef struct net_device {
    char name[ IFNAMSIZ ];
    int ref_count;

    int mtu;
    atomic_t flags;
    uint8_t dev_addr[ ETH_ADDR_LEN ];
    uint8_t ip_addr[ IPV4_ADDR_LEN ];
    uint8_t netmask[ IPV4_ADDR_LEN ];

    net_device_ops_t* ops;
    net_device_stats_t stats;

    void* private;

    thread_id rx_thread;
    packet_queue_t input_queue;

    lock_id arp_lock;
    hashtable_t arp_cache;
    hashtable_t arp_requests;
} net_device_t;

net_device_t* net_device_create( size_t priv_size );
int net_device_free( net_device_t* device );
int net_device_register( net_device_t* device );

net_device_t* net_device_get( char* name );
net_device_t* net_device_get_nth( int index );
int net_device_get_count( void );
int net_device_put( net_device_t* device );

int net_device_flags( net_device_t* device );
int net_device_running( net_device_t* device );

int net_device_carrier_ok( net_device_t* device );
int net_device_carrier_on( net_device_t* device );
int net_device_carrier_off( net_device_t* device );

int net_device_insert_packet( net_device_t* device, packet_t* packet );
int net_device_transmit( net_device_t* device, packet_t* packet );

void* net_device_get_private( net_device_t* device );

int net_device_init( void );
int net_device_start( void );

#endif /* _NETWORK_DEVICE_H_ */
