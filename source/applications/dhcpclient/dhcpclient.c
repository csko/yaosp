/* DHCP client
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "dhcpclient.h"

static char* argv0 = NULL;
static char* device = NULL;

static int sock = -1;
static char in_buffer[ 8192 ];

void init_packet( dhcp_msg_t* msg, uint8_t type ) {
    memset( msg, 0, sizeof( dhcp_msg_t ) );

    switch ( type ) {
        case DHCPDISCOVER :
        case DHCPREQUEST :
        case DHCPRELEASE :
        case DHCPINFORM :
            msg->op = BOOTREQUEST;
            break;
    }

    msg->htype = ETH_10MB;
    msg->hlen = ETH_10MB_LEN;
    msg->cookie = htonl(DHCP_MAGIC);

    /* The required DHCP message type option and the ending byte */

    msg->options[ 0 ] = DHCP_MSG_TYPE;
    msg->options[ 1 ] = DHCP_MSG_TYPE_LEN;
    msg->options[ 2 ] = type;
    msg->options[ 3 ] = DHCP_END;

    set_chaddr( msg );

    // TODO: set client ID, FQDN, hostname, vendor
}

void set_chaddr( dhcp_msg_t* msg ) {
    int fd;
    struct ifreq ifr;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );

    strncpy( ifr.ifr_name, device, IFNAMSIZ );
    ifr.ifr_name[ IFNAMSIZ - 1 ] = 0;

    if ( ioctl( fd, SIOCGIFHWADDR, &ifr ) == 0 ) {
        memcpy( msg->chaddr, ifr.ifr_hwaddr.sa_data, 6 );
        printf(
            "%s: client HW address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            argv0,
            msg->chaddr[ 0 ], msg->chaddr[ 1 ], msg->chaddr[ 2 ],
            msg->chaddr[ 3 ], msg->chaddr[ 4 ], msg->chaddr[ 5 ]
        );
    } else {
        fprintf( stderr, "%s: failed to get HW address.\n", argv0 );
    }
}

int send_packet( dhcp_msg_t* msg, uint32_t source_ip, int source_port,
                 uint32_t dest_ip, int dest_port, uint8_t* dest_arp, int if_index ) {
    int ret;
    struct sockaddr_in dest;

    if ( sock < 0 ) {
        return -1;
    }

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dest_ip;
    dest.sin_port = htons( dest_port );

    ret = sendto(
        sock, msg, sizeof( dhcp_msg_t ), MSG_NOSIGNAL,
        ( struct sockaddr* )&dest, sizeof( struct sockaddr_in )
    );

    printf( "%s: sendto() returned: %d.\n", argv0, ret );

    return 0;
}

int create_socket( void ) {
    struct sockaddr_in addr;

    sock = socket( AF_INET, SOCK_DGRAM, 0 );

    if ( sock < 0 ) {
        goto error1;
    }

    memset( &addr, 0, sizeof( struct sockaddr_in ) );

    addr.sin_family = AF_INET;
    addr.sin_port = htons( 68 );

    if ( bind( sock, ( struct sockaddr* )&addr, sizeof( struct sockaddr_in ) ) != 0 ) {
        goto error2;
    }

    if ( setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE, device, strlen( device ) ) != 0 ) {
        goto error2;
    }

    return 0;

 error2:
    close( sock );
    sock = -1;

 error1:
    return -1;
}

void send_discover( void ) {
    dhcp_msg_t msg;

    init_packet( &msg, DHCPDISCOVER );

    printf( "%s: sending DISCOVER.\n", argv0 );

    // TODO: register packet

    int if_index = 0; // TODO

    send_packet(
        &msg, INADDR_ANY, CLIENT_PORT, INADDR_BROADCAST,
        SERVER_PORT, MAC_BCAST_ADDR, if_index
    );
}

int interface_up( void ) {
    struct ifreq req;

    strncpy( req.ifr_name, device, IFNAMSIZ );
    req.ifr_name[ IFNAMSIZ - 1 ] = 0;

    if ( ioctl( sock, SIOCGIFFLAGS, &req ) != 0 ) {
        return -1;
    }

    if ( req.ifr_flags & IFF_UP ) {
        return 0;
    }

    req.ifr_flags |= IFF_UP;

    if ( ioctl( sock, SIOCSIFFLAGS, &req ) != 0 ) {
        return -1;
    }

    return 0;
}

int dhcp_mainloop( void ) {
    int size;
    socklen_t addr_len;
    struct sockaddr addr;

    while ( 1 ) {
        addr_len = sizeof( struct sockaddr );

        size = recvfrom( sock, in_buffer, sizeof( in_buffer ), MSG_NOSIGNAL, &addr, &addr_len );

        if ( size < 0 ) {
            fprintf( stderr, "%s: failed to recv incoming data.\n", argv0 );
            break;
        }

        printf( "%s: received %d bytes.\n", argv0, size );

        /* todo: handle the data */
    }

    return 0;
}

int main( int argc, char** argv ) {
    if ( argc != 2 ) {
        printf( "%s device\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    argv0 = argv[ 0 ];
    device = argv[ 1 ];

    printf( "%s: started.\n", argv0 );

    if ( create_socket() != 0 ) {
        fprintf( stderr, "%s: failed to create the UDP socket.\n", argv0 );
        return EXIT_FAILURE;
    }

    interface_up();
    send_discover();
    dhcp_mainloop();

    return 0;
}
