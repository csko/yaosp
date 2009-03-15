/* Network interface handling
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

#ifndef _NETWORK_INTERFACE_H_
#define _NETWORK_INTERFACE_H_

#include <types.h>
#include <thread.h>
#include <network/packet.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <lib/hashtable.h>

#include <arch/atomic.h>

typedef struct net_interface {
    hashitem_t hash;

    char name[ 16 ];
    atomic_t ref_count;

    int mtu;
    uint8_t hw_address[ ETH_ADDR_LEN ];
    uint8_t ip_address[ IPV4_ADDR_LEN ];

    int device;
    uint32_t flags;
    thread_id rx_thread;
    packet_queue_t* input_queue;
} net_interface_t;

int create_network_interfaces( void );

int network_interface_ioctl( int command, void* buffer, bool from_kernel );

int init_network_interfaces( void );

#endif /* _NETWORK_INTERFACE_H_ */
