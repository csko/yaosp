/* TCP packet handling
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

#ifndef _NETWORK_TCP_H_
#define _NETWORK_TCP_H_

#include <types.h>
#include <lock/mutex.h>
#include <lock/condition.h>
#include <vfs/vfs.h>
#include <network/socket.h>
#include <lib/hashtable.h>
#include <lib/circular.h>

#include <arch/atomic.h>

#define TCP_FIN 1
#define TCP_SYN 2
#define TCP_RST 4
#define TCP_PSH 8
#define TCP_ACK 16
#define TCP_URG 32

#define TCP_OPTION_MSS 0x2

#define TCP_HEADER_LEN 20

#define TCP_RECV_BUFFER_SIZE 32768
#define TCP_SEND_BUFFER_SIZE 32768

#define TCP_SYN_TIMEOUT   (75 * 1000 * 1000)

typedef struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_number;
    uint32_t ack_number;
    uint8_t data_offset;
    uint8_t ctrl_flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_pointer;
} __attribute__(( packed )) tcp_header_t;

typedef struct tcp_option_header {
    uint8_t kind;
    uint8_t length;
} __attribute__(( packed )) tcp_option_header_t;

typedef struct tcp_mss_option {
    uint8_t kind;
    uint8_t length;
    uint16_t mss;
} __attribute__(( packed )) tcp_mss_option_t;

typedef enum tcp_socket_state {
    TCP_STATE_CLOSED,
    TCP_STATE_SYN_SENT,
    TCP_STATE_ESTABLISHED
} tcp_socket_state_t;

typedef enum tcp_timer_type {
    TCP_TIMER_SYN_TIMEOUT = 0,
    TCP_TIMER_TRANSMIT,
    TCP_TIMER_COUNT
} tcp_timer_type_t;

struct tcp_socket;

typedef int tcp_timeout_callback_t( struct tcp_socket* tcp_socket );

typedef struct tcp_timer {
    bool running;
    uint64_t expire_time;
    tcp_timeout_callback_t* callback;
} tcp_timer_t;

typedef struct tcp_endpoint_key {
    uint8_t src_address[ IPV4_ADDR_LEN ];
    uint16_t src_port;
    uint8_t dest_address[ IPV4_ADDR_LEN ];
    uint16_t dest_port;
} __attribute__(( packed )) tcp_endpoint_key_t;

typedef struct tcp_socket {
    hashitem_t hash;

    atomic_t ref_count;
    tcp_endpoint_key_t endpoint_info;

    socket_t* socket;
    lock_id mutex;
    lock_id sync;
    tcp_socket_state_t state;

    int mss;

    lock_id rx_queue;
    circular_buffer_t rx_buffer;
    circular_pointer_t rx_user_data;
    circular_pointer_t rx_free_data;

    lock_id tx_queue;
    circular_buffer_t tx_buffer;
    uint32_t tx_last_unacked_seq;
    circular_pointer_t tx_first_unacked;
    uint32_t tx_last_sent_seq;
    circular_pointer_t tx_last_sent;
    circular_pointer_t tx_free_data;
    uint16_t tx_window_size;

    tcp_timer_t timers[ TCP_TIMER_COUNT ];

    select_request_t* first_read_select;
    select_request_t* first_write_select;
} tcp_socket_t;

int tcp_create_socket( socket_t* socket );
void put_tcp_endpoint( tcp_socket_t* tcp_socket );

int tcp_input( packet_t* packet );

int init_tcp( void );

#endif /* _NETWORK_TCP_H_ */
