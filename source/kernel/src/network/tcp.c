/* TCP packet handling
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

#include <config.h>

#ifdef ENABLE_NETWORK

#include <errno.h>
#include <console.h>
#include <macros.h>
#include <thread.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <network/tcp.h>
#include <network/ipv4.h>
#include <network/ethernet.h>
#include <network/route.h>
#include <network/device.h>
#include <lib/string.h>

#include <arch/interrupt.h>

static lock_id tcp_endpoint_lock;
static thread_id tcp_timer_thread;
static hashtable_t tcp_endpoint_table;

static uint16_t tcp_checksum( uint8_t* src_address, uint8_t* dest_address, uint8_t* tcp_header, size_t tcp_size ) {
    int i;
    uint16_t* data;
    uint32_t checksum;

    checksum = 0;
    data = ( uint16_t* )src_address;

    for ( i = 0; i < IPV4_ADDR_LEN / 2; i++, data++ ) {
        checksum += *data;
    }

    data = ( uint16_t* )dest_address;

    for ( i = 0; i < IPV4_ADDR_LEN / 2; i++, data++ ) {
        checksum += *data;
    }

    checksum += htonw( IP_PROTO_TCP );
    checksum += htonw( tcp_size );

    data = ( uint16_t* )tcp_header;

    for ( i = 0; i < tcp_size / 2; i++, data++ ) {
        checksum += *data;
    }

    if ( ( tcp_size % 2 ) != 0 ) {
        checksum += *( uint8_t* )data;
    }

    while ( checksum >> 16 ) {
        checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
    }

    return ~checksum;
}

static int tcp_send_packet( socket_t* socket, tcp_socket_t* tcp_socket, uint8_t flags, uint8_t* payload,
                            size_t payload_size, size_t option_size, uint32_t seq_number, uint32_t ack_number ) {
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
    DEBUG_LOG( "%s()\n", __FUNCTION__ );

    return 0;
}

static int tcp_connect( socket_t* socket, struct sockaddr* address, socklen_t addrlen ) {
    int error;
    uint8_t* ip;
    route_t* route;
    tcp_socket_t* tcp_socket;
    struct sockaddr_in* in_address;
    tcp_mss_option_t mss_option;
    tcp_timer_t* syn_timeout_timer;

    DEBUG_LOG( "%s()\n", __FUNCTION__ );

    tcp_socket = ( tcp_socket_t* )socket->data;
    in_address = ( struct sockaddr_in* )address;

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    if ( tcp_socket->state != TCP_STATE_CLOSED ) {
        switch ( tcp_socket->state ) {
            case TCP_STATE_SYN_SENT :
                error = -EALREADY;
                break;

            case TCP_STATE_ESTABLISHED :
                error = -EISCONN;
                break;

            default :
                error = -EINVAL;
                break;
        }

        mutex_unlock( tcp_socket->mutex );

        return error;
    }

    /* Find out the MSS value to the destination */

    ip = ( uint8_t* )&in_address->sin_addr.s_addr;
    route = route_find( ip );

    if ( route == NULL ) {
        mutex_unlock( tcp_socket->mutex );
        kprintf( WARNING, "net: no route for TCP endpoint: %d.%d.%d.%d.\n", ip[ 0 ], ip[ 1 ], ip[ 2 ], ip[ 3 ] );
        return -ENETUNREACH;
    }

    memcpy( socket->src_address, route->device->ip_addr, IPV4_ADDR_LEN );
    socket->src_port = ( get_system_time() % 65535 ) + 1;

    /* Calculate our MSS value */

    tcp_socket->mss = route->device->mtu - ( IPV4_HEADER_LEN + TCP_HEADER_LEN );

    route_put( route );

    /* Copy the endpoint information to the TCP socket structure */

    memcpy( tcp_socket->endpoint_info.src_address, socket->src_address, IPV4_ADDR_LEN );
    memcpy( tcp_socket->endpoint_info.dest_address, socket->dest_address, IPV4_ADDR_LEN );
    tcp_socket->endpoint_info.src_port = socket->src_port;
    tcp_socket->endpoint_info.dest_port = socket->dest_port;

    /* Build our MSS option */

    mss_option.kind = TCP_OPTION_MSS;
    mss_option.length = sizeof( tcp_mss_option_t );
    mss_option.mss = htonw( tcp_socket->mss );

    /* Put the socket to SYN_SENT state. */

    tcp_socket->state = TCP_STATE_SYN_SENT;

    /* Make the SYN timeout checker timer active. */

    syn_timeout_timer = &tcp_socket->timers[ TCP_TIMER_SYN_TIMEOUT ];

    syn_timeout_timer->running = true;
    syn_timeout_timer->expire_time = get_system_time() + TCP_SYN_TIMEOUT;

    mutex_unlock( tcp_socket->mutex );

    /* Put the new endpoint to the table */

    mutex_lock( tcp_endpoint_lock, LOCK_IGNORE_SIGNAL );
    hashtable_add( &tcp_endpoint_table, ( hashitem_t* )tcp_socket );
    mutex_unlock( tcp_endpoint_lock );

    /* Send the SYN packet */

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

    /* If this is a nonblocking socket, we're done */

    if ( tcp_socket->nonblocking ) {
        return -EINPROGRESS;
    }

    /* Wait until the connection is established */

    semaphore_lock( tcp_socket->sync, LOCK_IGNORE_SIGNAL, 1 );

    /* Calculate the return value */

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    if ( tcp_socket->state == TCP_STATE_ESTABLISHED ) {
        error = 0;
    } else {
        error = -socket_get_error( tcp_socket->socket );
    }

    mutex_unlock( tcp_socket->mutex );

    return error;
}

static int tcp_recvmsg( socket_t* socket, struct msghdr* msg, int flags ) {
    int i = 0;
    size_t data_read;
    size_t rx_data_size;
    tcp_socket_t* tcp_socket;

    data_read = 0;
    tcp_socket = ( tcp_socket_t* )socket->data;

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    if ( tcp_socket->state != TCP_STATE_ESTABLISHED ) {
        mutex_unlock( tcp_socket->mutex );
        return -ENOTCONN;
    }

    rx_data_size = circular_pointer_diff( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, &tcp_socket->rx_free_data );

    if ( tcp_socket->nonblocking ) {
        if ( rx_data_size == 0 ) {
            mutex_unlock( tcp_socket->mutex );

            return -EWOULDBLOCK;
        }
    } else {
        /* If there is no available data on the socket, then wait for someting to come ... */

        while ( rx_data_size == 0 ) {
            condition_wait( tcp_socket->rx_queue, tcp_socket->mutex );

            rx_data_size = circular_pointer_diff(
                &tcp_socket->rx_buffer,
                &tcp_socket->rx_user_data,
                &tcp_socket->rx_free_data
            );
        }
    }

    /* Read the available data from the socket */

    while ( ( rx_data_size > 0 ) && ( i < msg->msg_iovlen ) ) {
        size_t to_copy;
        struct iovec* iov = &msg->msg_iov[ i ];

        /* Copy the data */

        to_copy = MIN( rx_data_size, iov->iov_len );

        circular_buffer_read( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, iov->iov_base, to_copy );
        circular_pointer_move( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data, to_copy );

        i++;
        data_read += to_copy;

        /* Recalculate the available RX data size */

        rx_data_size = circular_pointer_diff( &tcp_socket->rx_buffer, &tcp_socket->rx_user_data,
                                              &tcp_socket->rx_free_data );
    }

    mutex_unlock( tcp_socket->mutex );

    return data_read;
}

static int tcp_sendmsg( socket_t* socket, struct msghdr* msg, int flags ) {
    int i;
    size_t data_written;
    tcp_socket_t* tcp_socket;

    data_written = 0;
    tcp_socket = ( tcp_socket_t* )socket->data;

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    if ( tcp_socket->state != TCP_STATE_ESTABLISHED ) {
        mutex_unlock( tcp_socket->mutex );
        return -ENOTCONN;
    }

    for ( i = 0; i < msg->msg_iovlen; i++ ) {
        size_t length;
        uint8_t* data;
        struct iovec* iov = &msg->msg_iov[ i ];

        data = ( uint8_t* )iov->iov_base;
        length = iov->iov_len;

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
                condition_wait( tcp_socket->tx_queue, tcp_socket->mutex );

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

            thread_wake_up( tcp_timer_thread );
        }
    }

    mutex_unlock( tcp_socket->mutex );

    return data_written;
}

static int tcp_getsockopt( socket_t* socket, int level, int optname, void* optval, socklen_t* optlen ) {
    int error;

    switch ( optname ) {
        default :
            kprintf( WARNING, "tcp_getsockopt(): unknown option: %d.\n", optname );
            error = -EINVAL;
            break;
    }

    return error;
}

static int tcp_setsockopt( socket_t* socket, int level, int optname, void* optval, socklen_t optlen ) {
    int error;

    switch ( optname ) {
        default :
            kprintf( WARNING, "tcp_setsockopt(): unknown option: %d.\n", optname );
            error = -EINVAL;
            break;
    }

    return error;
}

static int tcp_set_flags( socket_t* socket, int flags ) {
    tcp_socket_t* tcp_socket;

    tcp_socket = ( tcp_socket_t* )socket->data;

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    if ( flags & O_NONBLOCK ) {
        tcp_socket->nonblocking = 1;
    } else {
        tcp_socket->nonblocking = 0;
    }

    mutex_unlock( tcp_socket->mutex );

    return 0;
}

static int tcp_add_select_request( socket_t* socket, select_request_t* request ) {
    tcp_socket_t* tcp_socket;

    tcp_socket = ( tcp_socket_t* )socket->data;

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    switch ( ( int )request->type ) {
        case SELECT_READ :
            request->next = tcp_socket->first_read_select;
            tcp_socket->first_read_select = request;

            switch ( tcp_socket->state ) {
                case TCP_STATE_CLOSED :
                    request->ready = true;
                    semaphore_unlock( request->sync, 1 );
                    break;

                case TCP_STATE_ESTABLISHED :
                    if ( circular_pointer_diff(
                            &tcp_socket->rx_buffer,
                            &tcp_socket->rx_user_data,
                            &tcp_socket->rx_free_data ) > 0 ) {
                        request->ready = true;
                        semaphore_unlock( request->sync, 1 );
                    }

                    break;

                default :
                    break;
            }

            break;

        case SELECT_WRITE : {
            request->next = tcp_socket->first_write_select;
            tcp_socket->first_write_select = request;

            switch ( tcp_socket->state ) {
                case TCP_STATE_CLOSED :
                    request->ready = true;
                    semaphore_unlock( request->sync, 1 );
                    break;

                case TCP_STATE_ESTABLISHED : {
                    size_t free_size;

                    free_size = circular_buffer_size( &tcp_socket->tx_buffer ) - circular_pointer_diff(
                        &tcp_socket->tx_buffer,
                        &tcp_socket->tx_first_unacked,
                        &tcp_socket->tx_free_data
                    );

                    if ( free_size > 0 ) {
                        request->ready = true;
                        semaphore_unlock( request->sync, 1 );
                    }

                    break;
                }

                default :
                    break;
            }

            break;
        }

        case SELECT_EXCEPT :
            break;
    }

    mutex_unlock( tcp_socket->mutex );

    return 0;
}

static int tcp_remove_select_request( socket_t* socket, select_request_t* request ) {
    tcp_socket_t* tcp_socket;
    select_request_t* prev;
    select_request_t* tmp;

    tcp_socket = ( tcp_socket_t* )socket->data;

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    switch ( ( int )request->type ) {
        case SELECT_READ :
            prev = NULL;
            tmp = tcp_socket->first_read_select;

            while ( tmp != NULL ) {
                if ( tmp == request ) {
                    if ( prev == NULL ) {
                        tcp_socket->first_read_select = tmp->next;
                    } else {
                        prev->next = tmp->next;
                    }

                    break;
                }

                prev = tmp;
                tmp = tmp->next;
            }

            break;

        case SELECT_WRITE :
            prev = NULL;
            tmp = tcp_socket->first_write_select;

            while ( tmp != NULL ) {
                if ( tmp == request ) {
                    if ( prev == NULL ) {
                        tcp_socket->first_write_select = tmp->next;
                    } else {
                        prev->next = tmp->next;
                    }

                    break;
                }

                prev = tmp;
                tmp = tmp->next;
            }

            break;

        case SELECT_EXCEPT :
            break;
    }

    mutex_unlock( tcp_socket->mutex );

    return 0;
}

static socket_calls_t tcp_socket_calls = {
    .close = tcp_close,
    .connect = tcp_connect,
    .bind = NULL,
    .recvmsg = tcp_recvmsg,
    .sendmsg = tcp_sendmsg,
    .getsockopt = tcp_getsockopt,
    .setsockopt = tcp_setsockopt,
    .set_flags = tcp_set_flags,
    .add_select_request = tcp_add_select_request,
    .remove_select_request = tcp_remove_select_request
};

static int tcp_notify_write_listeners( tcp_socket_t* tcp_socket ) {
    select_request_t* request;

    request = tcp_socket->first_write_select;

    while ( request != NULL ) {
        request->ready = true;
        semaphore_unlock( request->sync, 1 );

        request = request->next;
    }

    return 0;
}

static int tcp_handle_syn_timeout( tcp_socket_t* tcp_socket ) {
    /* Shut down the SYN timeout timer. */

    tcp_socket->timers[ TCP_TIMER_SYN_TIMEOUT ].running = false;

    /* Connection failed as we did not receive the SYN|ACK in time. */

    tcp_socket->state = TCP_STATE_CLOSED;
    socket_set_error( tcp_socket->socket, ETIMEDOUT );

    if ( tcp_socket->nonblocking ) {
        tcp_notify_write_listeners( tcp_socket );
    } else {
        semaphore_unlock( tcp_socket->sync, 1 );
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
        TCP_PSH | TCP_ACK,
        ( uint8_t* )circular_pointer_get( &tcp_socket->tx_buffer, &tcp_socket->tx_last_sent ),
        data_to_send,
        0,
        tcp_socket->tx_last_sent_seq,
        tcp_socket->rx_last_received_seq + 1
    );

    circular_pointer_move( &tcp_socket->tx_buffer, &tcp_socket->tx_last_sent, data_to_send );

    tcp_socket->tx_window_size -= data_to_send;
    tcp_socket->tx_last_sent_seq += data_to_send;

    return 0;
}

int tcp_create_socket( socket_t* socket ) {
    int i;
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

    tcp_socket->mutex = mutex_create( "TCP socket mutex", MUTEX_NONE );

    if ( tcp_socket->mutex < 0 ) {
        goto error4;
    }

    tcp_socket->rx_queue = condition_create( "TCP RX queue" );

    if ( tcp_socket->rx_queue < 0 ) {
        goto error5;
    }

    tcp_socket->tx_queue = condition_create( "TCP TX queue" );

    if ( tcp_socket->tx_queue < 0 ) {
        goto error6;
    }

    tcp_socket->sync = semaphore_create( "TCP sync", 0 );

    if ( tcp_socket->sync < 0 ) {
        goto error7;
    }

    atomic_set( &tcp_socket->ref_count, 1 );
    tcp_socket->socket = socket;
    tcp_socket->mss = 0;
    tcp_socket->nonblocking = 0;
    tcp_socket->state = TCP_STATE_CLOSED;
    tcp_socket->tx_last_sent_seq = ( uint32_t )get_system_time();

    for ( i = 0; i < TCP_TIMER_COUNT; i++ ) {
        tcp_socket->timers[ i ].running = false;
    }

    tcp_socket->timers[ TCP_TIMER_SYN_TIMEOUT ].callback = tcp_handle_syn_timeout;
    tcp_socket->timers[ TCP_TIMER_TRANSMIT ].callback = tcp_handle_transmit;

    tcp_socket->first_read_select = NULL;
    tcp_socket->first_write_select = NULL;

    socket->data = ( void* )tcp_socket;
    socket->operations = &tcp_socket_calls;

    return 0;

 error7:
    condition_destroy( tcp_socket->tx_queue );

 error6:
    condition_destroy( tcp_socket->rx_queue );

 error5:
    mutex_destroy( tcp_socket->mutex );

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

    mutex_lock( tcp_endpoint_lock, LOCK_IGNORE_SIGNAL );

    tcp_socket = ( tcp_socket_t* )hashtable_get( &tcp_endpoint_table, ( const void* )&endpoint_key );

    if ( tcp_socket != NULL ) {
        ASSERT( atomic_get( &tcp_socket->ref_count ) > 0 );

        atomic_inc( &tcp_socket->ref_count );
    }

    mutex_unlock( tcp_endpoint_lock );

    return tcp_socket;
}

void put_tcp_endpoint( tcp_socket_t* tcp_socket ) {
    bool do_delete;

    do_delete = false;

    mutex_lock( tcp_endpoint_lock, LOCK_IGNORE_SIGNAL );

    if ( atomic_dec_and_test( &tcp_socket->ref_count ) ) {
        hashtable_remove( &tcp_endpoint_table, ( const void* )&tcp_socket->endpoint_info );

        do_delete = true;
    }

    mutex_unlock( tcp_endpoint_lock );

    if ( do_delete ) {
        mutex_destroy( tcp_socket->mutex );
        semaphore_destroy( tcp_socket->sync );
        condition_destroy( tcp_socket->rx_queue );
        condition_destroy( tcp_socket->tx_queue );

        destroy_circular_buffer( &tcp_socket->rx_buffer );
        destroy_circular_buffer( &tcp_socket->tx_buffer );

        kfree( tcp_socket );
    }
}

static int tcp_handle_syn_sent( tcp_socket_t* tcp_socket, packet_t* packet ) {
    int option_size;
    socket_t* socket;
    tcp_header_t* tcp_header;
    tcp_option_header_t* tcp_option_header;

    tcp_header = ( tcp_header_t* )packet->transport_data;
    socket = ( socket_t* )tcp_socket->socket;

    /* Check if we got something other than SYN|ACK. It means an error, definitely. */

    if ( ( tcp_header->ctrl_flags & ( TCP_SYN | TCP_ACK ) ) != ( TCP_SYN | TCP_ACK ) ) {
        tcp_socket->state = TCP_STATE_CLOSED;

        if ( tcp_header->ctrl_flags & TCP_RST ) {
            socket_set_error( socket, ECONNREFUSED );
        } else {
            socket_set_error( socket, EINVAL );
            tcp_send_reset( packet );
        }

        goto out;
    }

    /* Parse options in the TCP header */

    option_size = ( tcp_header->data_offset >> 4 ) * 4 - sizeof( tcp_header_t );

    if ( option_size > 0 ) {
        uint8_t* data;

        data = ( uint8_t* )( tcp_header + 1 );

        while ( option_size > 0 ) {
            tcp_option_header = ( tcp_option_header_t* )data;

            switch ( tcp_option_header->kind ) {
                case TCP_OPTION_MSS : {
                    tcp_mss_option_t* mss_option;

                    mss_option = ( tcp_mss_option_t* )tcp_option_header;

                    break;
                }
            }

            data += tcp_option_header->length;
            option_size -= tcp_option_header->length;
        }
    }

    tcp_socket->state = TCP_STATE_ESTABLISHED;
    tcp_socket->tx_window_size = ntohw( tcp_header->window_size );
    tcp_socket->tx_last_unacked_seq = tcp_socket->tx_last_sent_seq;

    tcp_socket->rx_last_received_seq = ntohl( tcp_header->seq_number );

    tcp_send_packet(
        socket,
        tcp_socket,
        TCP_ACK,
        NULL,
        0,
        0,
        tcp_socket->tx_last_sent_seq,
        tcp_socket->rx_last_received_seq + 1
    );

    tcp_socket->timers[ TCP_TIMER_SYN_TIMEOUT ].running = false;

    /* Connection has been established properly. Clear errors on the socket. */

    socket_set_error( tcp_socket->socket, 0 );

 out:
    if ( tcp_socket->nonblocking ) {
        tcp_notify_write_listeners( tcp_socket );
    } else {
        semaphore_unlock( tcp_socket->sync, 1 );
    }

    return 0;
}

static void tcp_notify_read_waiters( tcp_socket_t* tcp_socket ) {
    select_request_t* tmp;

    tmp = tcp_socket->first_read_select;

    while ( tmp != NULL ) {
        tmp->ready = true;
        semaphore_unlock( tmp->sync, 1 );

        tmp = tmp->next;
    }

    condition_broadcast( tcp_socket->rx_queue );
}

static int tcp_handle_established( tcp_socket_t* tcp_socket, packet_t* packet ) {
    uint8_t* data;
    size_t data_size;
    socket_t* socket;
    ipv4_header_t* ip_header;
    tcp_header_t* tcp_header;

    socket = ( socket_t* )tcp_socket->socket;
    ip_header = ( ipv4_header_t* )packet->network_data;
    tcp_header = ( tcp_header_t* )packet->transport_data;

    tcp_socket->rx_last_received_seq = ntohl( tcp_header->seq_number );

    /* Calculate the data pointer and the data size in the incoming packet */

    data = ( uint8_t* )tcp_header + ( tcp_header->data_offset >> 4 ) * 4;
    data_size = htonw( ip_header->packet_size ) -
        ( IPV4_HDR_SIZE( ip_header->version_and_size ) * 4 + ( tcp_header->data_offset >> 4 ) * 4 );

    /* Handle incoming data if we have any ... */

    if ( data_size > 0 ) {
        size_t handled_rx_size;

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

            tcp_notify_read_waiters( tcp_socket );
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
                condition_broadcast( tcp_socket->tx_queue );
            }
        }

        /* Handle new window size */

        new_window_size = ntohw( tcp_header->window_size );

        if ( ( tcp_socket->tx_window_size == 0 ) &&
             ( new_window_size > 0 ) ) {
            thread_wake_up( tcp_timer_thread );
        }

        tcp_socket->tx_window_size = new_window_size;
    }

    return 0;
}

int tcp_input( packet_t* packet ) {
    uint32_t transport_size;
    tcp_socket_t* tcp_socket;
    tcp_header_t* tcp_header;
    ipv4_header_t* ip_header;

    ip_header = ( ipv4_header_t* )packet->network_data;
    tcp_header = ( tcp_header_t* )packet->transport_data;
    transport_size = htonw( ip_header->packet_size ) - IPV4_HDR_SIZE( ip_header->version_and_size ) * 4;

    if ( tcp_checksum( ip_header->src_address, ip_header->dest_address,
                       ( uint8_t* )tcp_header, transport_size ) != 0 ) {
        kprintf( WARNING, "tcp_input(): Invalid TCP checksum, dropping packet.\n" );
        delete_packet( packet );
        return 0;
    }

    tcp_socket = get_tcp_endpoint( packet );

    if ( tcp_socket == NULL ) {
        tcp_send_reset( packet );
        goto out;
    }

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    switch ( ( int )tcp_socket->state ) {
        case TCP_STATE_SYN_SENT :
            tcp_handle_syn_sent( tcp_socket, packet );
            break;

        case TCP_STATE_ESTABLISHED :
            tcp_handle_established( tcp_socket, packet );
            break;
    }

    mutex_unlock( tcp_socket->mutex );

    put_tcp_endpoint( tcp_socket );

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

    mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );

    /* Check if we have something to send ... */

    data_to_send = circular_pointer_diff(
        &tcp_socket->tx_buffer,
        &tcp_socket->tx_last_sent,
        &tcp_socket->tx_free_data
    );

    found = ( ( data_to_send > 0 ) && ( tcp_socket->tx_window_size > 0 ) );

    mutex_unlock( tcp_socket->mutex );

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
            mutex_lock( tcp_endpoint_lock, LOCK_IGNORE_SIGNAL );
            tcp_socket = get_tcp_socket_for_work();
            mutex_unlock( tcp_endpoint_lock );

            if ( tcp_socket != NULL ) {
                mutex_lock( tcp_socket->mutex, LOCK_IGNORE_SIGNAL );
                tcp_handle_transmit( tcp_socket );
                mutex_unlock( tcp_socket->mutex );
            }
        } while ( tcp_socket != NULL );

        thread_sleep( 1 * 1000 * 1000 );
    }

    return 0;
}

__init int init_tcp( void ) {
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

    tcp_endpoint_lock = mutex_create( "TCP endpoint mutex", MUTEX_NONE );

    if ( tcp_endpoint_lock < 0 ) {
        destroy_hashtable( &tcp_endpoint_table );
        return tcp_endpoint_lock;
    }

    tcp_timer_thread = create_kernel_thread( "tcp_timer", PRIORITY_NORMAL, tcp_timer_thread_entry, NULL, 0 );

    if ( tcp_timer_thread < 0 ) {
        return -1;
    }

    thread_wake_up( tcp_timer_thread );

    return 0;
}

#endif /* ENABLE_NETWORK */
