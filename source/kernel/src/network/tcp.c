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
#include <mm/kmalloc.h>
#include <network/tcp.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <network/route.h>
#include <network/device.h>
#include <lib/string.h>

#include <arch/pit.h>

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

    ASSERT( ( tcp_size % 2 ) == 0 );

    data = ( uint16_t* )tcp_header;

    for ( i = 0; i < tcp_size / 2; i++ ) {
        checksum += data[ i ];
    }

    while ( checksum >> 16 ) {
        checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
    }

    return ~checksum;
}

static int tcp_send_packet( socket_t* socket, tcp_socket_t* tcp_socket, uint8_t flags, uint8_t* option_data, size_t option_size, uint32_t seq_number ) {
    uint8_t* data;
    packet_t* packet;
    tcp_header_t* tcp_header;

    packet = create_packet(
        ETH_HEADER_LEN +
        IPV4_HEADER_LEN +
        TCP_HEADER_LEN +
        option_size
    );

    if ( packet == NULL ) {
        return -ENOMEM;
    }

    data = packet->data + packet->size;

    data -= ( sizeof( tcp_header_t ) + option_size );
    packet->transport_data = data;

    tcp_header = ( tcp_header_t* )data;

    tcp_header->src_port = htonw( socket->src_port );
    tcp_header->dest_port = htonw( socket->dest_port );
    tcp_header->seq_number = htonl( seq_number );
    tcp_header->ack_number = htonl( tcp_socket->rx_window_low );
    tcp_header->data_offset = ( ( sizeof( tcp_header_t ) + option_size ) / 4 ) << 4;
    tcp_header->ctrl_flags = flags;
    tcp_header->window_size = htonw( 32768 );
    tcp_header->urgent_pointer = 0;

    if ( option_data != NULL ) {
        memcpy( ( void* )( tcp_header + 1 ), option_data, option_size );
    }

    tcp_header->checksum = 0;
    tcp_header->checksum = tcp_checksum(
        socket->src_address,
        socket->dest_address,
        ( uint8_t* )tcp_header,
        sizeof( tcp_header_t ) + option_size
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
        tcp_socket->tx_window_low
    );

    return 0;
}

static socket_calls_t tcp_socket_calls = {
    .close = tcp_close,
    .connect = tcp_connect
};

int tcp_create_socket( socket_t* socket ) {
    tcp_socket_t* tcp_socket;

    tcp_socket = ( tcp_socket_t* )kmalloc( sizeof( tcp_socket_t ) );

    if ( tcp_socket == NULL ) {
        goto error1;
    }

    tcp_socket->rx_buffer = ( uint8_t* )kmalloc( TCP_RECV_BUFFER_SIZE );

    if ( tcp_socket->rx_buffer == NULL ) {
        goto error2;
    }

    tcp_socket->tx_buffer = ( uint8_t* )kmalloc( TCP_SEND_BUFFER_SIZE );

    if ( tcp_socket->tx_buffer == NULL ) {
        goto error3;
    }

    tcp_socket->lock = create_semaphore( "TCP socket", SEMAPHORE_BINARY, 0, 1 );

    if ( tcp_socket->lock < 0 ) {
        goto error4;
    }

    tcp_socket->socket = socket;
    tcp_socket->mss = 0;
    tcp_socket->state = TCP_STATE_CLOSED;
    tcp_socket->rx_window_low = 0;
    tcp_socket->rx_window_high = 0;
    tcp_socket->tx_window_low = ( uint32_t )get_system_time();
    tcp_socket->tx_window_high = tcp_socket->tx_window_low;

    socket->data = ( void* )tcp_socket;
    socket->operations = &tcp_socket_calls;

    return 0;

error4:
    kfree( tcp_socket->tx_buffer );

error3:
    kfree( tcp_socket->rx_buffer );

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

    return 0;
}
