/* Network interface handling
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

#ifndef _NETWORK_INTERFACE_H_
#define _NETWORK_INTERFACE_H_

#include <types.h>
#include <thread.h>
#include <lock/mutex.h>
#include <network/packet.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <network/socket.h>
#include <lib/hashtable.h>

#include <arch/atomic.h>

#define IFHWADDRLEN 6
#define IFNAMSIZ    16

#define IFF_UP      ( 1 << 0 )

typedef struct net_interface {
    hashitem_t hash;

    char name[ 16 ];
    atomic_t ref_count;

    int mtu;
    uint8_t hw_address[ ETH_ADDR_LEN ];
    uint8_t ip_address[ IPV4_ADDR_LEN ];
    uint8_t netmask[ IPV4_ADDR_LEN ];
    uint8_t broadcast[ IPV4_ADDR_LEN ];

    int device;
    uint32_t flags;
    thread_id rx_thread;
    packet_queue_t* input_queue;

    lock_id arp_lock;
    hashtable_t arp_cache;
    hashtable_t arp_requests;
} net_interface_t;

struct ifreq {
    union {
        char ifrn_name[ IFNAMSIZ ];
    } ifr_ifrn;

    union {
        struct sockaddr ifru_addr;
        struct sockaddr ifru_dstaddr;
        struct sockaddr ifru_broadaddr;
        struct sockaddr ifru_netmask;
        struct sockaddr ifru_hwaddr;
        short ifru_flags;

        int ifru_ivalue;
        int ifru_mtu;
        char ifru_slave[ IFNAMSIZ ];
        char ifru_newname[ IFNAMSIZ ];
        char* ifru_data;
    } ifr_ifru;
};

struct ifconf {
    int ifc_len;
    union  {
        char* ifcu_buf;
        struct ifreq* ifcu_req;
    } ifc_ifcu;
};

extern lock_id interface_mutex;
extern hashtable_t interface_table;

int network_interface_ioctl( int command, void* buffer, bool from_kernel );

int init_network_interfaces( void );

#endif /* _NETWORK_INTERFACE_H_ */
