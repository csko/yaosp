/* Network interface configurator
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

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

#define CMD_NEED_PARAM ( 1 << 0 )

static int sock = -1;
static char* argv0 = NULL;

typedef int if_cmd_callback_t( struct ifreq* req, char* param );

typedef struct if_command {
    const char* name;
    int flags;
    if_cmd_callback_t* callback;
} if_command_t;

static int if_set_ip_address( struct ifreq* req, char* param ) {
    struct sockaddr_in* addr;

    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_addr;

    if ( inet_pton( AF_INET, param, &addr->sin_addr ) != 1 ) {
        fprintf( stderr, "%s: invalid IP address: %s.\n", argv0, param );
        return 0;
    }

    if ( ioctl( sock, SIOCSIFADDR, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set IP address.\n", argv0 );
    }

    return 0;
}

static int if_set_netmask( struct ifreq* req, char* param ) {
    struct sockaddr_in* addr;

    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_netmask;

    if ( inet_pton( AF_INET, param, &addr->sin_addr ) != 1 ) {
        fprintf( stderr, "%s: invalid network mask: %s.\n", argv0, param );
        return 0;
    }

    if ( ioctl( sock, SIOCSIFNETMASK, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set network mask.\n", argv0 );
    }

    return 0;
}

static int if_set_broadcast( struct ifreq* req, char* param ) {
    struct sockaddr_in* addr;

    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_broadaddr;

    if ( inet_pton( AF_INET, param, &addr->sin_addr ) != 1 ) {
        fprintf( stderr, "%s: invalid broadcast address: %s.\n", argv0, param );
        return 0;
    }

    if ( ioctl( sock, SIOCSIFBRDADDR, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set broadcast address.\n", argv0 );
    }

    return 0;
}

static int if_up( struct ifreq* req, char* param ) {
    if ( ioctl( sock, SIOCGIFFLAGS, req ) != 0 ) {
        fprintf( stderr, "%s: failed to get interface flags.\n", argv0 );
        return 0;
    }

    if ( req->ifr_flags & IFF_UP ) {
        return 0;
    }

    req->ifr_flags |= IFF_UP;

    if ( ioctl( sock, SIOCSIFFLAGS, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set interface flags.\n", argv0 );
    }

    return 0;
}

static int if_down( struct ifreq* req, char* param ) {
    if ( ioctl( sock, SIOCGIFFLAGS, req ) != 0 ) {
        fprintf( stderr, "%s: failed to get interface flags.\n", argv0 );
        return 0;
    }

    if ( ( req->ifr_flags & IFF_UP ) == 0 ) {
        return 0;
    }

    req->ifr_flags &= ~IFF_UP;

    if ( ioctl( sock, SIOCSIFFLAGS, req ) != 0 ) {
        fprintf( stderr, "%s: failed to set interface flags.\n", argv0 );
    }

    return 0;
}

static if_command_t if_commands[] = {
    { "ip",        CMD_NEED_PARAM, if_set_ip_address },
    { "netmask",   CMD_NEED_PARAM, if_set_netmask },
    { "broadcast", CMD_NEED_PARAM, if_set_broadcast },
    { "up",        0,              if_up },
    { "down",      0,              if_down },
    { NULL,        0,              NULL }
};

static char* format_size( char* buf, size_t len, uint64_t size ) {
    if ( size < 1024 ) {
        snprintf( buf, len, "%llu b", size );
    } else if ( size < ( 1024 * 1024 ) ) {
        snprintf( buf, len, "%llu Kb", size / 1024 );
    } else if ( size < ( 1024 * 1024 * 1024 ) ) {
        snprintf( buf, len, "%llu Mb", size / ( 1024 * 1024 ) );
    } else {
        snprintf( buf, len, "%llu Gb", size / ( 1024 * 1024 * 1024 ) );
    }

    return buf;
}

static int show_network_interface( struct ifreq* req ) {
    char ip[ 32 ];
    if_stat_t stat;
    char rx_size[ 32 ];
    char tx_size[ 32 ];
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

    printf( " Mask: %s ", ip );

    ioctl( sock, SIOCGIFBRDADDR, req );
    addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_broadaddr;
    inet_ntop( AF_INET, &addr->sin_addr, ip, sizeof( ip ) );

    printf( " Broadcast: %s\n", ip );

    ioctl( sock, SIOCGIFMTU, req );
    printf( "      MTU: %d", req->ifr_ifru.ifru_mtu );

    ioctl( sock, SIOCGIFFLAGS, req );
    printf( " Flags:" );

    if ( req->ifr_flags == 0 ) {
        printf( " -" );
    } else {
        if ( req->ifr_flags & IFF_RUNNING ) { printf( " running" ); }
        if ( req->ifr_flags & IFF_UP ) { printf( " up" ); }
    }

    printf( "\n" );

    req->ifr_ifru.ifru_data = ( char* )&stat;
    ioctl( sock, SIOCGIFSTAT, req );

    printf( "      RX packets: %u errors: %u dropped: %u\n", stat.rx_packets, stat.rx_errors, stat.rx_dropped );
    printf( "      TX packets: %u errors: %u dropped: %u\n", stat.tx_packets, stat.tx_errors, stat.tx_dropped );
    printf(
        "      RX bytes: %llu (%s) TX bytes: %llu (%s)\n",
        stat.rx_bytes, format_size( rx_size, sizeof( rx_size ), stat.rx_bytes ),
        stat.tx_bytes, format_size( tx_size, sizeof( tx_size ), stat.tx_bytes )
    );

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

    if_conf.ifc_len = if_count;
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
    } else {
        struct ifreq req;
        char* ifname = argv[ 1 ];

        strncpy( req.ifr_ifrn.ifrn_name, ifname, IFNAMSIZ - 1 );
        req.ifr_ifrn.ifrn_name[ IFNAMSIZ - 1 ] = 0;

        if ( ioctl( sock, SIOCGIFFLAGS, &req ) != 0 ) {
            fprintf( stderr, "%s: invalid interface: %s.\n", argv0, req.ifr_ifrn.ifrn_name );
            return EXIT_FAILURE;
        }

        if ( argc == 2 ) {
            show_network_interface( &req );
        } else {
            int i = 2;

            while ( i < argc ) {
                int j;
                char* cmd = argv[ i ];

                for ( j = 0; if_commands[ j ].name != NULL; j++ ) {
                    if_command_t* if_cmd = &if_commands[ j ];

                    if ( strcmp( if_cmd->name, cmd ) == 0 ) {
                        if ( if_cmd->flags & CMD_NEED_PARAM ) {
                            if ( i == argc - 1 )  {
                                fprintf( stderr, "%s: missing parameter.\n", argv0 );
                                return EXIT_FAILURE;
                            }

                            if_cmd->callback( &req, argv[ ++i ] );
                        } else {
                            if_cmd->callback( &req, NULL );
                        }

                        break;
                    }
                }

                i++;
            }
        }
    }

    close( sock );

    return EXIT_SUCCESS;
}
