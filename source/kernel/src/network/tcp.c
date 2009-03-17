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

#include <errno.h>
#include <console.h>
#include <macros.h>
#include <thread.h>
#include <mm/kmalloc.h>
#include <network/tcp.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <network/route.h>
#include <network/device.h>
#include <lib/string.h>

#include <arch/pit.h>

#include <arch/interrupt.h>

static thread_id tcp_timer_thread;
static hashtable_t tcp_endpoint_table;
static semaphore_id tcp_endpoint_lock;

static uint16_t tcp_checksum( uint8_t* src_address, uint8_t* dest_address, uint8_t* tcp_header, size_t tcp_size ) {
    int i;
    uint16_t* data;
    uint32_t checksum;

    checksum = 0;
    data = ( uint16_t* )src_address;

    for ( i = 0; i < IPV4_ADDR_LEN / 2; i++ ) {
        checksum += data[ i ];
    }

    data = ( uint16_t* )dest_address;

    for ( i = 0; i < IPV4_ADDR_LEN / 2; i++ ) {
        checksum += data[ i ];
    }

    checksum += htonw( IP_PROTO_TCP );
    checksum += htonw( tcp_size );

    data = ( uint16_t* )tcp_header;

    for ( i = 0; i < tcp_size / 2; i++ ) {
        checksum += data[ i ];
    }

    if ( ( tcp_size % 2 ) != 0 ) {
        uint8_t* tmp;
        uint16_t tmp_data;

        tmp = ( uint8_t* )( &data[ tcp_size / 2 ] );
        tmp_data = ( uint16_t )*tmp;

        checksum += tmp_data;
    }

    while ( checksum >> 16 ) {
        checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
    }

    return ~checksum;
}

static int tcp_send_packet( socket_t* socket, tcp_socket_t* tcp_socket, uint8_t flags, uint8_t* payload, size_t payload_size, size_t option_size, uint32_t seq_number, uint32_t ack_number ) {
    uint8_t* data;
    packet_t* packet;
    size_t window_size;
    tcp_header_t* tcp_header;

    packet = create_packet(
        ETH_HEADER_LEN +
        IPV4_HEADER_LEN +
        TCP_HEADER_LEN +
        payload_size
    );

    if ( packet == NULL ) {
        return -ENOMEM;
    }

    data = packet->data + packet->size;

    data -= ( sizeof( tcp_header_t ) + payload_size );
    packet->transport_data = data;

    tcp_header = ( tcp_header_t* )data;

    /* Calculate our window size */

    window_size =
        circular_buffer_size( &tcp_socket->rx_buffer ) -
        circular_pointer_diff( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, &tcp_socket->rx_free_data );
    window_size = MIN( window_size, 65535 );

    tcp_header->src_port = htonw( socket->src_port );
    tcp_header->dest_port = htonw( socket->dest_port );
    tcp_header->seq_number = htonl( seq_number );
    tcp_header->ack_number = htonl( ack_number );
    tcp_header->data_offset = ( ( sizeof( tcp_header_t ) + option_size ) / 4 ) << 4;
    tcp_header->ctrl_flags = flags;
    tcp_header->window_size = htonw( window_size );
    tcp_header->urgent_pointer = 0;

    if ( payload != NULL ) {
        memcpy( ( void* )( tcp_header + 1 ), payload, payload_size );
    }

    tcp_header->checksum = 0;
    tcp_header->checksum = tcp_checksum(
        socket->src_address,
        socket->dest_address,
        ( uint8_t* )tcp_header,
        sizeof( tcp_header_t ) + payload_size
    );

    return ipv4_send_packet( socket->dest_address, packet, IP_PROTO_TCP );
}

static int tcp_send_reset( packet_t* packet ) {
    uint8_t* data;
    packet_t* reply;
    ipv4_header_t* ip_header;
    tcp_header_t* tcp_header;
    tcp_header_t* tcp_reply_header;

    ip_header = ( ipv4_header_t* )packet->network_data;
    tcp_header = ( tcp_header_t* )packet->transport_data;

    /* Don't send reset to a reset packet */

    if ( tcp_header->ctrl_flags & TCP_RST ) {
        return 0;
    }

    reply = create_packet(
        ETH_HEADER_LEN + IPV4_HEADER_LEN + TCP_HEADER_LEN
    );

    if ( reply == NULL ) {
        return -ENOMEM;
    }

    data = reply->data + reply->size;
    data -= sizeof( tcp_header_t );

    reply->transport_data = data;
    tcp_reply_header = ( tcp_header_t* )data;

    /* Build the reply TCP header */

    tcp_reply_header->src_port = tcp_header->dest_port;
    tcp_reply_header->dest_port = tcp_header->src_port;
    tcp_reply_header->seq_number = 0;
    tcp_reply_header->ack_number = htonl( ntohl( tcp_header->seq_number ) + 1 );
    tcp_reply_header->data_offset = ( sizeof( tcp_header_t ) / 4 ) << 4;
    tcp_reply_header->ctrl_flags = TCP_RST | TCP_ACK;
    tcp_reply_header->window_size = 0;
    tcp_reply_header->urgent_pointer = 0;

    /* Calculate the TCP checksum */

    tcp_reply_header->checksum = 0;
    tcp_reply_header->checksum = tcp_checksum(
        ip_header->src_address,
        ip_header->dest_address,
        ( uint8_t* )tcp_reply_header,
        sizeof( tcp_header_t )
    );

    return ipv4_send_packet( ip_header->src_address, reply, IP_PROTO_TCP );
}

static int tcp_close( socket_t* socket ) {
    return 0;
}

static int tcp_connect( socket_t* socket, struct sockaddr* address, socklen_t addrlen ) {
    route_t* route;
    tcp_socket_t* tcp_socket;
    struct sockaddr_in* in_address;
    tcp_mss_option_t mss_option;

    tcp_socket = ( tcp_socket_t* )socket->data;
    in_address = ( struct sockaddr_in* )address;

    LOCK( tcp_socket->lock );

    if ( tcp_socket->state != TCP_STATE_CLOSED ) {
        UNLOCK( tcp_socket->lock );

        return -EINVAL;
    }

    tcp_socket->state = TCP_STATE_SYN_SENT;

    route = find_route( ( uint8_t* )&in_address->sin_addr.s_addr );

    if ( route == NULL ) {
        UNLOCK( tcp_socket->lock );

        kprintf( "NET: No route for TCP endpoint!\n" );

        return -EINVAL;
    }

    memcpy( socket->src_address, route->interface->ip_address, IPV4_ADDR_LEN );
    socket->src_port = 12345;

    tcp_socket->mss = route->interface->mtu - ( IPV4_HEADER_LEN + TCP_HEADER_LEN );

    put_route( route );

    mss_option.kind = 0x2;
    mss_option.length = 0x4;
    mss_option.mss = htonw( tcp_socket->mss );

    UNLOCK( tcp_socket->lock );

    LOCK( tcp_endpoint_lock );

    hashtable_add( &tcp_endpoint_table, ( hashitem_t* )tcp_socket );

    UNLOCK( tcp_endpoint_lock );

    tcp_send_packet(
        socket,
        tcp_socket,
        TCP_SYN,
        ( uint8_t* )&mss_option,
        sizeof( tcp_mss_option_t ),
        sizeof( tcp_mss_option_t ),
        tcp_socket->tx_last_sent_seq++,
        0
    );

    LOCK( tcp_socket->sync );

    return 0;
}

static int tcp_read( socket_t* socket, void* data, size_t length ) {
    size_t to_copy;
    size_t rx_data_size;
    tcp_socket_t* tcp_socket;

    tcp_socket = ( tcp_socket_t* )socket->data;

    LOCK( tcp_socket->lock );

    rx_data_size = circular_pointer_diff( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, &tcp_socket->rx_free_data );

    while ( rx_data_size == 0 ) {
        UNLOCK( tcp_socket->lock );
        LOCK( tcp_socket->rx_queue );
        LOCK( tcp_socket->lock );

        rx_data_size = circular_pointer_diff( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, &tcp_socket->rx_free_data );
    }

    to_copy = MIN( rx_data_size, length );

    circular_buffer_read( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, data, to_copy );
    circular_pointer_move( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, to_copy );

    UNLOCK( tcp_socket->lock );

    return to_copy;
}

static int tcp_write( socket_t* socket, const void* _data, size_t length ) {
    uint8_t* data;
    size_t data_written;
    tcp_socket_t* tcp_socket;

    data_written = 0;
    data = ( uint8_t* )_data;
    tcp_socket = ( tcp_socket_t* )socket->data;

    LOCK( tcp_socket->lock );

    while ( length > 0 ) {
        size_t to_copy;
        size_t free_size;

        /* Wait until there is free space in the send buffer */

        free_size = circular_buffer_size( &tcp_socket->tx_buffer ) - circular_pointer_diff(
            &tcp_socket->tx_buffer,
            &tcp_socket->tx_first_unacked,
            &tcp_socket->tx_free_data
        );

        while ( free_size == 0 ) {
            UNLOCK( tcp_socket->lock );
            LOCK( tcp_socket->tx_queue );
            LOCK( tcp_socket->lock );

            free_size = circular_buffer_size( &tcp_socket->tx_buffer ) - circular_pointer_diff(
                &tcp_socket->tx_buffer,
                &tcp_socket->tx_first_unacked,
                &tcp_socket->tx_free_data
            );
        }

        /* Copy the data to the send buffer */

        to_copy = MIN( length, free_size );

        circular_buffer_write( &tcp_socket->tx_buffer, &tcp_socket->tx_free_data, data, to_copy );
        circular_pointer_move( &tcp_socket->tx_buffer, &tcp_socket->tx_free_data, to_copy );

        data += to_copy;
        length -= to_copy;
        data_written += to_copy;

        /* Wake up the TCP timer thread to transmit the data */

        wake_up_thread( tcp_timer_thread );
    }

    UNLOCK( tcp_socket->lock );

    return data_written;
}

static socket_calls_t tcp_socket_calls = {
    .close = tcp_close,
    .connect = tcp_connect,
    .read = tcp_read,
    .write = tcp_write
};

int tcp_create_socket( socket_t* socket ) {
    int error;
    tcp_socket_t* tcp_socket;

    tcp_socket = ( tcp_socket_t* )kmalloc( sizeof( tcp_socket_t ) );

    if ( tcp_socket == NULL ) {
        goto error1;
    }

    error = init_circular_buffer( &tcp_socket->rx_buffer, TCP_RECV_BUFFER_SIZE );

    if ( error < 0 ) {
        goto error2;
    }

    init_circular_pointer( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, 0 );
    init_circular_pointer( &tcp_socket->rx_buffer, &tcp_socket->rx_free_data, 0 );

    error = init_circular_buffer( &tcp_socket->tx_buffer, TCP_SEND_BUFFER_SIZE );

    if ( error < 0 ) {
        goto error3;
    }

    init_circular_pointer( &tcp_socket->tx_buffer, &tcp_socket->tx_first_unacked, 0 );
    init_circular_pointer( &tcp_socket->tx_buffer, &tcp_socket->tx_last_sent, 0 );
    init_circular_pointer( &tcp_socket->tx_buffer, &tcp_socket->tx_free_data, 0 );

    tcp_socket->lock = create_semaphore( "TCP socket", SEMAPHORE_BINARY, 0, 1 );

    if ( tcp_socket->lock < 0 ) {
        goto error4;
    }

    tcp_socket->rx_queue = create_semaphore( "TCP RX queue", SEMAPHORE_COUNTING, 0, 0 );

    if ( tcp_socket->rx_queue < 0 ) {
        goto error5;
    }

    tcp_socket->tx_queue = create_semaphore( "TCP TX queue", SEMAPHORE_COUNTING, 0, 0 );

    if ( tcp_socket->tx_queue < 0 ) {
        goto error6;
    }

    tcp_socket->sync = create_semaphore( "TCP sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( tcp_socket->sync < 0 ) {
        goto error7;
    }

    tcp_socket->socket = socket;
    tcp_socket->mss = 0;
    tcp_socket->state = TCP_STATE_CLOSED;
    tcp_socket->tx_last_sent_seq = ( uint32_t )get_system_time();

    socket->data = ( void* )tcp_socket;
    socket->operations = &tcp_socket_calls;

    return 0;

error7:
    delete_semaphore( tcp_socket->tx_queue );

error6:
    delete_semaphore( tcp_socket->rx_queue );

error5:
    delete_semaphore( tcp_socket->lock );

error4:
    destroy_circular_buffer( &tcp_socket->tx_buffer );

error3:
    destroy_circular_buffer( &tcp_socket->rx_buffer );

error2:
    kfree( tcp_socket );

error1:
    return -ENOMEM;
}

static tcp_socket_t* get_tcp_endpoint( packet_t* packet ) {
    tcp_socket_t* tcp_socket;
    ipv4_header_t* ip_header;
    tcp_header_t* tcp_header;
    tcp_endpoint_key_t endpoint_key;

    ip_header = ( ipv4_header_t* )packet->network_data;
    tcp_header = ( tcp_header_t* )packet->transport_data;

    memcpy( &endpoint_key.dest_address, ip_header->src_address, IPV4_ADDR_LEN );
    endpoint_key.dest_port = ntohw( tcp_header->src_port );
    memcpy( &endpoint_key.src_address, ip_header->dest_address, IPV4_ADDR_LEN );
    endpoint_key.src_port = ntohw( tcp_header->dest_port );

    LOCK( tcp_endpoint_lock );

    tcp_socket = ( tcp_socket_t* )hashtable_get( &tcp_endpoint_table, ( const void* )&endpoint_key );

    UNLOCK( tcp_endpoint_lock );

    return tcp_socket;
}

static int tcp_handle_syn_sent( tcp_socket_t* tcp_socket, packet_t* packet ) {
    socket_t* socket;
    tcp_header_t* tcp_header;

    tcp_header = ( tcp_header_t* )packet->transport_data;
    socket = ( socket_t* )tcp_socket->socket;

    if ( ( tcp_header->ctrl_flags & ( TCP_SYN | TCP_ACK ) ) != ( TCP_SYN | TCP_ACK ) ) {
        tcp_send_reset( packet );
        return -EINVAL;
    }

    tcp_socket->state = TCP_STATE_ESTABLISHED;
    tcp_socket->tx_window_size = ntohw( tcp_header->window_size );
    tcp_socket->tx_last_unacked_seq = tcp_socket->tx_last_sent_seq;

    tcp_send_packet(
        socket,
        tcp_socket,
        TCP_ACK,
        NULL,
        0,
        0,
        tcp_socket->tx_last_sent_seq,
        ntohl( tcp_header->seq_number ) + 1
    );

    UNLOCK( tcp_socket->sync );

    return 0;
}

static int tcp_handle_established( tcp_socket_t* tcp_socket, packet_t* packet ) {
    socket_t* socket;
    tcp_header_t* tcp_header;

    socket = ( socket_t* )tcp_socket->socket;
    tcp_header = ( tcp_header_t* )packet->transport_data;

    if ( tcp_header->ctrl_flags & TCP_PSH ) {
        uint8_t* data;
        size_t data_size;
        size_t handled_rx_size;

        /* Calculate the data pointer and the data size in the incoming packet */

        data = ( uint8_t* )tcp_header + ( tcp_header->data_offset >> 4 ) * 4;
        data_size = ( packet->size -
            ( ( ( uint32_t )packet->transport_data - ( uint32_t )packet->data ) +
              ( tcp_header->data_offset >> 4 ) * 4 ) );

        /* Calculate the free size in the RX buffer */

        handled_rx_size =
            circular_buffer_size( &tcp_socket->rx_buffer ) -
            circular_pointer_diff( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, &tcp_socket->rx_free_data );
        handled_rx_size = MIN( handled_rx_size, data_size );

        if ( handled_rx_size > 0 ) {
            /* Copy the data to the RX buffer */

            circular_buffer_write( &tcp_socket->rx_buffer, &tcp_socket->rx_free_data, data, handled_rx_size );
            circular_pointer_move( &tcp_socket->rx_buffer, &tcp_socket->rx_free_data, handled_rx_size );

            /* Wake up waiters */

            UNLOCK( tcp_socket->rx_queue );
        }

        /* ACK the received data */

        tcp_send_packet(
            socket,
            tcp_socket,
            TCP_ACK,
            NULL,
            0,
            0,
            tcp_socket->tx_last_sent_seq,
            ntohl( tcp_header->seq_number ) + handled_rx_size
        );
    }

    if ( tcp_header->ctrl_flags & TCP_ACK ) {
        uint32_t ack_number;
        uint32_t acked_data_size;
        uint32_t unacked_data_size;
        uint16_t new_window_size;

        /* TODO: Handle seq number wrapping here! */

        ack_number = ntohl( tcp_header->ack_number );
        acked_data_size = ack_number - tcp_socket->tx_last_unacked_seq;
        unacked_data_size = circular_pointer_diff(
            &tcp_socket->tx_buffer,
            &tcp_socket->tx_first_unacked,
            &tcp_socket->tx_last_sent
        );

        if ( acked_data_size > unacked_data_size ) {
            /* TODO */
            return 0;
        }

        /* Handle ACKed data */

        tcp_socket->tx_last_unacked_seq = ack_number + 1;

        if ( acked_data_size > 0 ) {
            bool tx_buffer_full;

            tx_buffer_full = ( circular_buffer_size( &tcp_socket->tx_buffer ) == circular_pointer_diff(
                &tcp_socket->tx_buffer,
                &tcp_socket->tx_first_unacked,
                &tcp_socket->tx_free_data
            ) );

            circular_pointer_move( &tcp_socket->tx_buffer, &tcp_socket->tx_first_unacked, acked_data_size );

            if ( tx_buffer_full ) {
                UNLOCK( tcp_socket->tx_queue );
            }
        }

        /* Handle new window size */

        new_window_size = ntohw( tcp_header->window_size );

        if ( ( tcp_socket->tx_window_size == 0 ) &&
             ( new_window_size > 0 ) ) {
            wake_up_thread( tcp_timer_thread );
        }

        tcp_socket->tx_window_size = new_window_size;
    }

    return 0;
}

static int tcp_handle_transmit( tcp_socket_t* tcp_socket ) {
    socket_t* socket;
    size_t data_to_send;

    socket = ( socket_t* )tcp_socket->socket;

    ASSERT( tcp_socket->tx_window_size > 0 );

    data_to_send = circular_pointer_diff(
        &tcp_socket->tx_buffer,
        &tcp_socket->tx_last_sent,
        &tcp_socket->tx_free_data
    );

    ASSERT( data_to_send > 0 );

    data_to_send = MIN( data_to_send, tcp_socket->tx_window_size );

    tcp_send_packet(
        socket,
        tcp_socket,
        TCP_PSH,
        ( uint8_t* )circular_pointer_get( &tcp_socket->tx_buffer, &tcp_socket->tx_last_sent ),
        data_to_send,
        0,
        tcp_socket->tx_last_sent_seq,
        0
    );

    circular_pointer_move( &tcp_socket->tx_buffer, &tcp_socket->tx_last_sent, data_to_send );

    tcp_socket->tx_window_size -= data_to_send;
    tcp_socket->tx_last_sent_seq += data_to_send;

    return 0;
}

int tcp_input( packet_t* packet ) {
    tcp_socket_t* tcp_socket;

    /* TODO: Check the TCP checksum */

    tcp_socket = get_tcp_endpoint( packet );

    if ( tcp_socket == NULL ) {
        tcp_send_reset( packet );
        goto out;
    }

    LOCK( tcp_socket->lock );

    switch ( ( int )tcp_socket->state ) {
        case TCP_STATE_SYN_SENT :
            tcp_handle_syn_sent( tcp_socket, packet );
            break;

        case TCP_STATE_ESTABLISHED :
            tcp_handle_established( tcp_socket, packet );
            break;
    }

    UNLOCK( tcp_socket->lock );

out:
    delete_packet( packet );

    return 0;
}

static void* tcp_endpoint_key( hashitem_t* item ) {
    tcp_socket_t* tcp_socket;

    tcp_socket = ( tcp_socket_t* )item;

    return &tcp_socket->socket->src_address[ 0 ];
}

static uint32_t tcp_endpoint_hash( const void* key ) {
    return hash_number(
        ( uint8_t* )key,
        2 * IPV4_ADDR_LEN + 2 * sizeof( uint16_t )
    );
}

static bool tcp_endpoint_compare( const void* key1, const void* key2 ) {
    return ( memcmp( key1, key2, 2 * IPV4_ADDR_LEN + 2 * sizeof( uint16_t ) ) == 0 );
}

static int get_tcp_socket_iterator( hashitem_t* hash, void* _data ) {
    bool found;
    size_t data_to_send;
    tcp_socket_t* tcp_socket;
    tcp_socket_t** data;

    tcp_socket = ( tcp_socket_t* )hash;
    data = ( tcp_socket_t** )_data;

    LOCK( tcp_socket->lock );

    /* Check if we have something to send ... */

    data_to_send = circular_pointer_diff(
        &tcp_socket->tx_buffer,
        &tcp_socket->tx_last_sent,
        &tcp_socket->tx_free_data
    );

    found = ( ( data_to_send > 0 ) && ( tcp_socket->tx_window_size > 0 ) );

    UNLOCK( tcp_socket->lock );

    if ( found ) {
        *data = tcp_socket;
        return -1;
    }

    return 0;
}

static tcp_socket_t* get_tcp_socket_for_work( void ) {
    int error;
    tcp_socket_t* tcp_socket = NULL;

    error = hashtable_iterate( &tcp_endpoint_table, get_tcp_socket_iterator, ( void* )&tcp_socket );

    if ( error < 0 ) {
        ASSERT( tcp_socket != NULL );
    }

    return tcp_socket;
}

static int tcp_timer_thread_entry( void* data ) {
    tcp_socket_t* tcp_socket;

    while ( 1 ) {
        do {
            LOCK( tcp_endpoint_lock );
            tcp_socket = get_tcp_socket_for_work();
            UNLOCK( tcp_endpoint_lock );

            if ( tcp_socket != NULL ) {
                LOCK( tcp_socket->lock );
                tcp_handle_transmit( tcp_socket );
                UNLOCK( tcp_socket->lock );
            }
        } while ( tcp_socket != NULL );

        sleep_thread( 1 * 1000 * 1000 );
    }

    return 0;
}

int init_tcp( void ) {
    int error;

    error = init_hashtable(
        &tcp_endpoint_table,
        64,
        tcp_endpoint_key,
        tcp_endpoint_hash,
        tcp_endpoint_compare
    );

    if ( error < 0 ) {
        return error;
    }

    tcp_endpoint_lock = create_semaphore( "TCP endpoint lock", SEMAPHORE_BINARY, 0, 1 );

    if ( tcp_endpoint_lock < 0 ) {
        destroy_hashtable( &tcp_endpoint_table );
        return tcp_endpoint_lock;
    }

    tcp_timer_thread = create_kernel_thread( "tcp_timer", PRIORITY_NORMAL, tcp_timer_thread_entry, NULL, 0 );

    if ( tcp_timer_thread < 0 ) {
        return -1;
    }

    wake_up_thread( tcp_timer_thread );

    return 0;
}
