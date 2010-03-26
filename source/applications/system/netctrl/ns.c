/* yaOSp network control application
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
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <yaosp/config.h>

#include "netctrl.h"

static int ns_add( int argc, char** argv ) {
    int ret;
    struct in_addr addr;

    if ( argc != 1 ) {
        fprintf( stderr, "%s: parameter is missing for ns add.\n", argv0 );
        return -1;
    }

    if ( inet_pton( AF_INET, argv[0], &addr ) != 1 ) {
        fprintf( stderr, "%s: invalid name server address: %s.\n", argv0, argv[0] );
        return -1;
    }

    ret = ycfg_add_child( "network/nameservers", argv[0] );

    if ( ret != 0 ) {
        fprintf( stderr, "%s: failed to add name server: %s.\n", argv0, strerror(ret) );
        return -1;
    }

    return 0;
}

static int ns_del( int argc, char** argv ) {
    int ret;
    struct in_addr addr;

    if ( argc != 1 ) {
        fprintf( stderr, "%s: parameter is missing for ns del.\n", argv0 );
        return -1;
    }

    if ( inet_pton( AF_INET, argv[0], &addr ) != 1 ) {
        fprintf( stderr, "%s: invalid name server address: %s.\n", argv0, argv[0] );
        return -1;
    }

    ret = ycfg_del_child( "network/nameservers", argv[0] );

    if ( ret != 0 ) {
        fprintf( stderr, "%s: failed to delete name server: %s.\n", argv0, strerror(ret) );
        return -1;
    }

    return 0;
}

static int ns_list( int argc, char** argv ) {
    int i;
    char** ns_list;

    if ( ycfg_list_children( "network/nameservers", &ns_list ) != 0 ) {
        fprintf( stderr, "%s: failed to list name servers.\n", argv0 );
        return -1;
    }

    if ( ns_list[0] == NULL ) {
        printf( "No name servers.\n" );
        return 0;
    }

    printf( "Name servers:\n" );

    for ( i = 0; ns_list[i] != NULL; i++ ) {
        printf( "  %s\n", ns_list[i] );
        free( ns_list[i] );
    }

    free( ns_list );

    return 0;
}

static ctrl_command_t ns_commands[] = {
    { "add", "adds a new name server", ns_add },
    { "del", "deletes an existing name server", ns_del },
    { "list", "lists the current name servers", ns_list },
    { NULL, NULL }
};

ctrl_subsystem_t ns = {
    .name = "ns",
    .commands = ns_commands
};
