/* UDP packet handling
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

#ifndef _NETWORK_UDP_H_
#define _NETWORK_UDP_H_

#include <types.h>
#include <network/packet.h>
#include <network/socket.h>
#include <network/interface.h>
#include <lib/hashtable.h>

#define UDP_HEADER_LEN 8

struct udp_socket;

typedef struct udp_header {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t size;
    uint16_t checksum;
} __attribute__(( packed )) udp_header_t;

typedef struct udp_port {
    hashitem_t hash;

    int local_port;
    struct udp_socket* udp_socket;
} udp_port_t;

typedef struct udp_socket {
    lock_id lock;
    int ref_count;

    int nonblocking;

    lock_id rx_condition;
    packet_queue_t rx_queue;

    udp_port_t* port;
    char bind_to_device[ IFNAMSIZ ];

    select_request_t* first_read_select;
} udp_socket_t;

int udp_create_socket( socket_t* socket );

int udp_input( packet_t* packet );

int init_udp( void );

#endif /* _NETWORK_UDP_H_ */
