/* Network route configurator
 *
 * Copyright (c) 2010 Zoltan Kovacs
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
#include <net/route.h>
#include <arpa/inet.h>

#define MAX_ROUTE_COUNT 256

static int sock = -1;
static char* argv0 = NULL;

static int route_format_flags( struct rtabentry* entry, char* buf, size_t size ) {
    if ( entry->rt_flags == 0 ) {
        snprintf( buf, size, "-" );
    } else {
        snprintf(
            buf, size, "%s%s",
            ( entry->rt_flags & RTF_UP ? "U" : "" ),
            ( entry->rt_flags & RTF_GATEWAY ? "G" : "" )
        );
    }

    return 0;
}

static int route_format_gateway( struct rtabentry* entry, char* buf, size_t size ) {
    if ( entry->rt_flags & RTF_GATEWAY ) {
        inet_ntop( AF_INET, &entry->rt_gateway, buf, size );
    } else {
        buf[0] = 0; /* empty string */
    }

    return 0;
}

static int list_routes( void ) {
    int i;
    struct rttable* table;
    struct rtabentry* entry;

    table = (struct rttable*)malloc( sizeof(struct rttable) + MAX_ROUTE_COUNT * sizeof(struct rtabentry) );

    if ( table == NULL ) {
        fprintf( stderr, "%s: failed to list routes: out of memory.\n", argv0 );
        return -1;
    }

    table->rtt_count = MAX_ROUTE_COUNT;

    if ( ioctl( sock, SIOCGETRTAB, table ) != 0 ) {
        fprintf( stderr, "%s: failed to get route table: %s.\n", argv0, strerror(errno) );
        return -1;
    }

    entry = (struct rtabentry*)( table + 1 );

    printf( "Destination     Gateway         Genmask         Flags Interface\n" );

    for ( i = 0; i < table->rtt_count; i++, entry++ ) {
        char buf[32];

        inet_ntop( AF_INET, &entry->rt_dst, buf, sizeof(buf) );
        printf( "%-15s ", buf );
        route_format_gateway( entry, buf, sizeof(buf) );
        printf( "%-15s ", buf );
        inet_ntop( AF_INET, &entry->rt_genmask, buf, sizeof(buf) );
        printf( "%-15s ", buf );
        route_format_flags( entry, buf, sizeof(buf) );
        printf( "%-5s ", buf );
        printf( "%s", entry->rt_dev );
        printf( "\n" );
    }

    free(table);

    return 0;
}

static int add_route( int argc, char** argv ) {
    int i;
    int ip_found = 0;
    struct rtentry entry;

    if ( ( argc < 2 ) ||
         ( argc % 2 != 0 ) ) {
        return -1;
    }

    memset( &entry, 0, sizeof(struct rtentry) );

    entry.rt_flags = RTF_UP;

    for ( i = 0; i < argc; i += 2 ) {
        if ( strcmp( argv[i], "ip" ) == 0 ) {
            ip_found = 1;

            if ( inet_pton( AF_INET, argv[i+1], &entry.rt_dst ) != 1 ) {
                return -1;
            }
        } else if ( strcmp( argv[i], "mask" ) == 0 ) {
            if ( inet_pton( AF_INET, argv[i+1], &entry.rt_genmask ) != 1 ) {
                return -1;
            }
        } else if ( strcmp( argv[i], "gw" ) == 0 ) {
            if ( inet_pton( AF_INET, argv[i+1], &entry.rt_gateway ) != 1 ) {
                return -1;
            }

            entry.rt_flags |= RTF_GATEWAY;
        } else if ( strcmp( argv[i], "dev" ) == 0 ) {
            entry.rt_dev = argv[i+1];
        }
    }

    if ( !ip_found ) {
        return -1;
    }

    if ( ioctl( sock, SIOCADDRT, &entry ) != 0 ) {
        fprintf( stderr, "%s: failed to add route: %s.\n", argv0, strerror(errno) );
    }

    return 0;
}

static int del_route( int argc, char** argv ) {
    return 0;
}

static int print_usage( void ) {
    printf( "Usage:\n" );
    printf( "  %s add ip <address> [mask <netmask>] [gw <gateway>] [dev <device>]\n", argv0 );
    printf( "  %s list\n", argv0 );

    return 0;
}

int main( int argc, char** argv ) {
    argv0 = argv[ 0 ];

    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if ( sock < 0 ) {
        fprintf( stderr, "%s: failed to create socket: %s.\n", argv0, strerror(errno) );
        return EXIT_FAILURE;
    }

    if ( argc == 1 ) {
        list_routes();
    } else {
        int ret;
        char* command = argv[1];

        if ( strcmp( command, "list" ) == 0 ) {
            list_routes();
            ret = 0;
        } else if ( strcmp( command, "add" ) == 0 ) {
            ret = add_route( argc - 2, &argv[2] );
        } else if ( strcmp( command, "del" ) == 0 ) {
            ret = del_route( argc - 2, &argv[2] );
        } else {
            ret = -1;
        }

        if ( ret != 0 ) {
            print_usage();
        }
    }

    close( sock );

    return EXIT_SUCCESS;
}
