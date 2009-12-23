/* Network interface configurator
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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

static int sock = -1;

static char* argv0 = NULL;

static int show_network_interface( struct ifreq* req ) {
    char ip[ 32 ];
    struct sockaddr_in* addr;

    ioctl( sock, SIOCGIFHWADDR, req );
    printf(
        "%-5s HW addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
        req->ifr_ifrn.ifrn_name,
        req->ifr_ifru.ifru_hwaddr.sa_data[ 0 ] & 0xff, req->ifr_ifru.ifru_hwaddr.sa_data[ 1 ] & 0xff,
        req->ifr_ifru.ifru_hwaddr.sa_data[ 2 ] & 0xff, req->ifr_ifru.ifru_hwaddr.sa_data[ 3 ] & 0xff,
        req->ifr_ifru.ifru_hwaddr.sa_data[ 4 ] & 0xff, req->ifr_ifru.ifru_hwaddr.sa_data[ 5 ] & 0xff
    );

    ioctl( sock, SIOCGIFADDR, req );
    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_addr;
    inet_ntop( AF_INET, &addr->sin_addr, ip, sizeof( ip ) );

    printf( "      IP addr: %s", ip );

    ioctl( sock, SIOCGIFNETMASK, req );
    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_netmask;
    inet_ntop( AF_INET, &addr->sin_addr, ip, sizeof( ip ) );

    printf( " Mask: %s\n", ip );

    ioctl( sock, SIOCGIFMTU, req );
    printf( "      MTU: %d", req->ifr_ifru.ifru_mtu );

    ioctl( sock, SIOCGIFFLAGS, req );
    printf( " Flags: -" );
    /* todo */
    printf( "\n" );

    return 0;
}

static int list_network_interfaces( void ) {
    int i;
    int if_count;
    struct ifconf if_conf;
    struct ifreq* if_table;

    if_count = ioctl( sock, SIOCGIFCOUNT, NULL );

    if ( if_count < 0 ) {
        fprintf( stderr, "%s: failed to get interface count.\n", argv0 );
        return 0;
    } else if ( if_count == 0 ) {
        printf( "%s: there are no network interfaces.\n", argv0 );
        return 0;
    }

    if_table = ( struct ifreq* )malloc( sizeof( struct ifreq ) * if_count );

    if ( if_table == NULL ) {
        fprintf( stderr, "%s: no memory for interface table.\n", argv0 );
        return 0;
    }

    if_conf.ifc_len = 0; /* todo */
    if_conf.ifc_ifcu.ifcu_req = if_table;

    if ( ioctl( sock, SIOCGIFCONF, ( void* )&if_conf ) != 0 ) {
        fprintf( stderr, "%s: failed to get interface configuration.\n", argv0 );
        free( if_table );
        return 0;
    }

    for ( i = 0; i < if_count; i++ ) {
        struct ifreq* req;

        req = &if_table[ i ];
        show_network_interface( req );
    }

    return 0;
}

int main( int argc, char** argv ) {
    argv0 = argv[ 0 ];

    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if ( sock < 0 ) {
        fprintf( stderr, "%s: failed to create socket: %s.\n", argv0, strerror( errno ) );
        return EXIT_FAILURE;
    }

    if ( argc == 1 ) {
        list_network_interfaces();
    }

    close( sock );

    return EXIT_SUCCESS;
}
