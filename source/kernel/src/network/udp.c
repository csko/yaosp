/* UDP packet handling
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

#include <errno.h>
#include <console.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <network/udp.h>
#include <network/device.h>
#include <network/packet.h>
#include <network/route.h>
#include <lib/bitmap.h>

static bitmap_t udp_port_table;

static lock_id udp_endpoint_lock;
static hashtable_t udp_endpoint_table;

static uint16_t udp_checksum( uint8_t* src_address, uint8_t* dest_address, uint8_t* udp_header, size_t udp_size ) {
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

    checksum += htonw( IP_PROTO_UDP );
    checksum += htonw( udp_size );

    data = ( uint16_t* )udp_header;

    for ( i = 0; i < udp_size / 2; i++ ) {
        checksum += data[ i ];
    }

    if ( ( udp_size % 2 ) != 0 ) {
        uint8_t* tmp;
        uint16_t tmp_data;

        tmp = ( uint8_t* )( &data[ udp_size / 2 ] );
        tmp_data = ( uint16_t )*tmp;

        checksum += tmp_data;
    }

    while ( checksum >> 16 ) {
        checksum = ( checksum & 0xFFFF ) + ( checksum >> 16 );
    }

    return ~checksum;
}

static udp_socket_t* udp_get_endpoint( int port ) {
    udp_port_t* udp_port;

    mutex_lock( udp_endpoint_lock, LOCK_IGNORE_SIGNAL );

    udp_port = ( udp_port_t* )hashtable_get( &udp_endpoint_table, ( const void* )&port );

    if ( udp_port != NULL ) {
        udp_port->udp_socket->ref_count++;
    }

    mutex_unlock( udp_endpoint_lock );

    return udp_port->udp_socket;
}

static int udp_put_endpoint( udp_socket_t* socket ) {
    int do_destroy;

    mutex_lock( udp_endpoint_lock, LOCK_IGNORE_SIGNAL );

    do_destroy = ( --socket->ref_count == 0 );

    if ( do_destroy ) {
        hashtable_remove( &udp_endpoint_table, ( const void* )&socket->port->local_port );
    }

    mutex_unlock( udp_endpoint_lock );

    if ( do_destroy ) {
        /* todo */
    }

    return 0;
}

static int udp_close( socket_t* socket ) {
    return 0;
}

static int udp_bind( socket_t* socket, struct sockaddr* _addr, socklen_t size ) {
    int port;
    udp_port_t* udp_port;
    udp_socket_t* udp_socket;
    struct sockaddr_in* addr;

    udp_socket = ( udp_socket_t* )socket->data;
    addr = ( struct sockaddr_in* )_addr;

    if ( udp_socket->port != NULL ) {
        return -EINVAL;
    }

    port = ntohw( addr->sin_port );

    if ( port == 0 ) {
        /* Find a free port for the socket. */

        port = bitmap_first_not_set_in_range( &udp_port_table, 1024, 65535 );

        if ( port < 0 ) {
            return -ENOSPC; /* todo: what is the proper error code here? */
        }

        bitmap_set( &udp_port_table, port );
    } else {
        /* Check if the specified port is free or not. */

        if ( bitmap_get( &udp_port_table, port ) != 0 ) {
            return -EADDRINUSE;
        }
    }

    /* Create the port for the socket. */

    udp_port = ( udp_port_t* )kmalloc( sizeof( udp_port_t ) );

    if ( udp_port == NULL ) {
        bitmap_unset( &udp_port_table, port );
        return -ENOMEM;
    }

    udp_port->local_port = port;
    udp_port->udp_socket = udp_socket;

    udp_socket->port = udp_port;

    hashtable_add( &udp_endpoint_table, ( hashitem_t* )udp_port );

    return 0;
}

static int udp_recvmsg( socket_t* socket, struct msghdr* msg, int flags ) {
    int i;
    int ret;
    int data_size;
    uint8_t* data;
    packet_t* packet;
    udp_socket_t* udp_socket;
    udp_header_t* udp_header;

    udp_socket = ( udp_socket_t* )socket->data;

    mutex_lock( udp_socket->lock, LOCK_IGNORE_SIGNAL );

    if ( udp_socket->port == NULL ) {
        ret = -ENOTCONN;
        goto out;
    }

    if ( udp_socket->nonblocking ) {
        if ( packet_queue_is_empty( &udp_socket->rx_queue ) ) {
            ret = -EWOULDBLOCK;
            goto out;
        }
    } else {
        while ( packet_queue_is_empty( &udp_socket->rx_queue ) ) {
            condition_wait( udp_socket->rx_condition, udp_socket->lock );
        }

        if ( packet_queue_is_empty( &udp_socket->rx_queue ) ) {
            ret = -EINTR;
            goto out;
        }

    }

    /* Get the first packet from the RX queue of the socket */

    packet = packet_queue_pop_head( &udp_socket->rx_queue, 0 );
    ASSERT( packet != NULL );

    /* Copy the data from the packet */

    udp_header = ( udp_header_t* )packet->transport_data;

    data = ( uint8_t* )( udp_header + 1 );
    data_size = htonw( udp_header->size );

    ret = data_size;

    for ( i = 0; i < msg->msg_iovlen; i++ ) {
        int to_copy;
        struct iovec* iov = &msg->msg_iov[ i ];

        to_copy = MIN( iov->iov_len, data_size );

        memcpy( iov->iov_base, data, to_copy );

        data += to_copy;
        data_size -= to_copy;
    }

    /* Delete the packet */

    delete_packet( packet );

 out:
    mutex_unlock( udp_socket->lock );

    return ret;
}

static int udp_sendmsg( socket_t* socket, struct msghdr* msg, int flags ) {
    int i;
    int error;
    uint8_t* ip;
    uint8_t* data;
    packet_t* packet;
    route_t* route;
    int bind_to_device;
    size_t payload_size;
    udp_socket_t* udp_socket;
    udp_header_t* udp_header;
    struct sockaddr_in* address;

    udp_socket = ( udp_socket_t* )socket->data;
    bind_to_device = ( strlen( udp_socket->bind_to_device ) > 0 );

    address = ( struct sockaddr_in* )msg->msg_name;
    ip = ( uint8_t* )&address->sin_addr.s_addr;

    if ( bind_to_device ) {
        route = find_device_route( udp_socket->bind_to_device );

        if ( route == NULL ) {
            kprintf( WARNING, "net: no route for device: %s.\n", udp_socket->bind_to_device );
            error = -ENETUNREACH;
            goto error1;
        }
    } else {
        route = find_route( ip );

        if ( route == NULL ) {
            kprintf( WARNING, "net: no route for UDP endpoint: %d.%d.%d.%d.\n", ip[ 0 ], ip[ 1 ], ip[ 2 ], ip[ 3 ] );
            error = -ENETUNREACH;
            goto error1;
        }
    }

    /* Find a free local port for the socket if not yet found. */

    if ( udp_socket->port == NULL ) {
        struct sockaddr addr;

        memset( &addr, 0, sizeof( struct sockaddr ) );

        if ( udp_bind( socket, &addr, sizeof( struct sockaddr ) ) != 0 ) {
            error = -EAGAIN;
            goto error2;
        }
    }

    ASSERT( udp_socket->port != NULL );

    /* Send the data */

    payload_size = 0;

    for ( i = 0; i < msg->msg_iovlen; i++ ) {
        struct iovec* iov;

        iov = &msg->msg_iov[ i ];
        payload_size += iov->iov_len;
    }

    /* todo: how to handle UDP packages with size bigger than 65535? */

    if ( payload_size > 65535 ) {
        error = -E2BIG;
        goto error2;
    }

    packet = create_packet(
        ETH_HEADER_LEN +
        IPV4_HEADER_LEN +
        UDP_HEADER_LEN +
        payload_size
    );

    if ( packet == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    data = packet->data + packet->size;
    data -= ( sizeof( udp_header_t ) + payload_size );
    packet->transport_data = data;

    udp_header = ( udp_header_t* )data;

    udp_header->src_port = htonw( udp_socket->port->local_port );
    udp_header->dst_port = address->sin_port;
    udp_header->size = htonw( sizeof( udp_header_t ) + payload_size );

    data = ( uint8_t* )( udp_header + 1 );

    for ( i = 0; i < msg->msg_iovlen; i++ ) {
        struct iovec* iov = &msg->msg_iov[ i ];

        memcpy( data, iov->iov_base, iov->iov_len );
        data += iov->iov_len;
    }

    udp_header->checksum = 0;
    udp_header->checksum = udp_checksum(
        route->device->ip_addr,
        ( uint8_t* )&address->sin_addr.s_addr,
        ( uint8_t* )udp_header,
        sizeof( udp_header_t ) + payload_size
    );

    if ( bind_to_device ) {
        error = ipv4_send_packet_via_route( route, ip, packet, IP_PROTO_UDP );
    } else {
        error = ipv4_send_packet( ip, packet, IP_PROTO_UDP );
    }

    put_route( route );

    if ( error < 0 ) {
        goto error3;
    }

    return payload_size;

 error3:
    delete_packet( packet );

 error2:
    put_route( route );

 error1:
    return error;
}

static int udp_getsockopt( socket_t* socket, int level, int optname, void* optval, socklen_t* optlen ) {
    return 0;
}

static int udp_setsockopt( socket_t* socket, int level, int optname, void* optval, socklen_t optlen ) {
    int error;
    udp_socket_t* udp_socket;

    udp_socket = ( udp_socket_t* )socket->data;

    switch ( optname ) {
        case SO_BINDTODEVICE : {
            if ( ( optval == NULL ) ||
                 ( optlen > IFNAMSIZ ) ) {
                error = -EINVAL;
                break;
            }

            strncpy( udp_socket->bind_to_device, optval, IFNAMSIZ );
            udp_socket->bind_to_device[ IFNAMSIZ - 1 ] = 0;

            error = 0;

            break;
        }

        default :
            error = -EINVAL;
            break;
    }

    return error;
}

static socket_calls_t udp_socket_calls = {
    .close = udp_close,
    .connect = NULL,
    .bind = udp_bind,
    .recvmsg = udp_recvmsg,
    .sendmsg = udp_sendmsg,
    .getsockopt = udp_getsockopt,
    .setsockopt = udp_setsockopt,
    .set_flags = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int udp_create_socket( socket_t* socket ) {
    int error;
    udp_socket_t* udp_socket;

    udp_socket = ( udp_socket_t* )kmalloc( sizeof( udp_socket_t ) );

    if ( udp_socket == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    udp_socket->lock = mutex_create( "UDP socket lock", MUTEX_NONE );

    if ( udp_socket->lock < 0 ) {
        error = -ENOMEM;
        goto error2;
    }

    udp_socket->rx_condition = condition_create( "UDP RX queue" );

    if ( udp_socket->rx_condition < 0 ) {
        error = -ENOMEM;
        goto error3;
    }

    packet_queue_init( &udp_socket->rx_queue );

    udp_socket->ref_count = 1;
    udp_socket->port = NULL;
    udp_socket->nonblocking = 0;

    memset( udp_socket->bind_to_device, 0, IFNAMSIZ );

    socket->data = ( void* )udp_socket;
    socket->operations = &udp_socket_calls;

    return 0;

 error3:
    mutex_destroy( udp_socket->lock );

 error2:
    kfree( udp_socket );

 error1:
    return error;
}

int udp_input( packet_t* packet ) {
    uint32_t transport_size;
    udp_header_t* udp_header;
    udp_socket_t* udp_socket;
    ipv4_header_t* ip_header;


    ip_header = ( ipv4_header_t* )packet->network_data;
    udp_header = ( udp_header_t* )packet->transport_data;

    transport_size = packet->size - ( ( uint32_t )packet->transport_data - ( uint32_t )packet->data );

    if ( udp_checksum( ip_header->src_address, ip_header->dest_address,
                       ( uint8_t* )udp_header, transport_size ) != 0 ) {
        kprintf( WARNING, "udp_input(): invalid UDP checksum, dropping packet.\n" );
        delete_packet( packet );
        return 0;
    }

    udp_socket = udp_get_endpoint( htonw( udp_header->dst_port ) );

    if ( udp_socket == NULL ) {
        delete_packet( packet );
        return 0;
    }

    mutex_lock( udp_socket->lock, LOCK_IGNORE_SIGNAL );
    packet_queue_insert( &udp_socket->rx_queue, packet );
    mutex_unlock( udp_socket->lock );
    condition_signal( udp_socket->rx_condition );

    udp_put_endpoint( udp_socket );

    return 0;
}

static void* udp_endpoint_key( hashitem_t* item ) {
    udp_port_t* port;

    port = ( udp_port_t* )item;

    return ( void* )&port->local_port;
}

int init_udp( void ) {
    int error;

    error = init_bitmap( &udp_port_table, 65536 );

    if ( error < 0 ) {
        return error;
    }

    error = init_hashtable( &udp_endpoint_table, 256, udp_endpoint_key, hash_int, compare_int );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

#endif /* ENABLE_NETWORK */
