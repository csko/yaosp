/* IRC client
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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <yaosp/debug.h>

#include "../core/event.h"
#include "../core/eventmanager.h"

static int s;
static event_t irc_read;

static int irc_handle_incoming( event_t* event ) {
    int data;
    char buffer[ 512 ];

    data = read( s, b, sizeof( buffer ) );

    return 0;
}

void init_irc( void ) {
    int error;
    char buffer[ 256 ];

    struct sockaddr_in address;

    s = socket( AF_INET, SOCK_STREAM, 0 );

    if ( s < 0 ) {
        return;
    }

    address.sin_family = AF_INET;
    //inet_aton( "94.125.176.255", &address.sin_addr ); /* irc.atw.hu */
    inet_aton( "157.181.1.129", &address.sin_addr ); /* irc.elte.hu */
    //inet_aton( "192.168.1.101", &address.sin_addr );
    address.sin_port = htons( 6667 );

    error = connect( s, ( struct sockaddr* )&address, sizeof( struct sockaddr_in ) );

    if ( error < 0 ) {
        return;
    }

    snprintf( buffer, sizeof( buffer ), "NICK gisz0\r\nUSER gisz0 SERVER irc.atw.hu :giszo from yaOSp\r\n" );

    write( s, buffer, strlen( buffer ) );

    irc_read.fd = s;
    irc_read.events[ EVENT_READ ].interested = 1;
    irc_read.events[ EVENT_READ ].callback = irc_handle_incoming;
    irc_read.events[ EVENT_WRITE ].interested = 0;
    irc_read.events[ EVENT_EXCEPT ].interested = 0;

    event_manager_add_event( &irc_read );
}
