/* IRC client
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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
#include "../ui/view.h"
#include "../ui/ui.h"

char* my_nick;

static int s;
static event_t irc_read;

static size_t input_size = 0;
static char* input_buffer = NULL;
/* TODO: proper parameter handling */
static void irc_handle_line( char* line ) {
    char* tmp;
    char* cmd;
    char* chan;

    view_add_text( &server_view, line );

    cmd = strchr( line, ' ' );

    if ( cmd == NULL ) {
        return;
    }

    cmd++;

    chan = strchr( cmd, ' ' );

    if ( chan == NULL ) {
        return;
    }

    *chan++ = 0;

    tmp = strchr( chan, ' ' );

    if ( tmp == NULL ) {
        return;
    }

    *tmp = 0;

    if ( strcmp( cmd, "PRIVMSG" ) == 0 ) {
        char* msg;
        char* nick;
        char buf[ 256 ];
        view_t* channel;

        msg = strchr( tmp + 1, ':' );

        if ( msg == NULL ) {
            return;
        }

        msg++;

        tmp = strchr( line + 1, '!' );

        if ( tmp == NULL ) {
            return;
        }

        *tmp = 0;
        nick = line + 1;

        channel = ui_get_channel( chan );

        if ( channel == NULL ) {
            return;
        }

        snprintf( buf, sizeof( buf ), "<%s> %s", nick, msg );

        view_add_text( channel, buf );
    }else if ( strcmp( cmd, "PING" ) == 0 ) { /* TODO: this is not working */
        char buf[64];
        snprintf( buf, sizeof(buf), "PONG :%s\r\n", chan );
        write( s, buf, strlen(buf) );
    }
}

static int irc_handle_incoming( event_t* event ) {
    int size;
    char buffer[ 512 ];

    size = read( s, buffer, sizeof( buffer ) - 1 );

    if ( size > 0 ) {
        char* tmp;
        char* start;
        size_t length;

        buffer[ size ] = 0;

        tmp = ( char* )malloc( input_size + size + 1 );

        if ( tmp != NULL ) {
            if ( input_size > 0 ) {
                memcpy( tmp, input_buffer, input_size );
            }

            if ( input_buffer != NULL ) {
                free( input_buffer );
            }

            input_buffer = tmp;
        }

        memcpy( input_buffer + input_size, buffer, size );

        input_size += size;
        tmp[ input_size ] = 0;

        start = input_buffer;
        tmp = strstr( start, "\r\n" );

        while ( tmp != NULL ) {
            *tmp = 0;

            irc_handle_line( start );

            start = tmp + 2;
            tmp = strstr( start, "\r\n" );
        }

        if ( start > input_buffer ) {
            length = strlen( start );

            if ( start == 0 ) {
                free( input_buffer );
            } else {
                memmove( input_buffer, start, length );
                input_buffer = ( char* )realloc( input_buffer, length );
            }

            input_size = length;
        }
    }

    return 0;
}

int irc_join_channel( const char* channel ) {
    char buf[ 128 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "JOIN %s\r\n", channel );

    write( s, buf, length );

    return 0;
}

int irc_send_privmsg( const char* channel, const char* message ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "PRIVMSG %s :%s\r\n", channel, message );

    write( s, buf, length );

    return 0;
}

int irc_raw_command( const char* command ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "%s\r\n", command );

    write( s, buf, length );

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

    snprintf( buffer, sizeof( buffer ), "NICK %s\r\nUSER %s SERVER irc.atw.hu :yaOSp IRC client\r\n", my_nick, my_nick );
    /* TODO: handle "nickname already in use" */

    write( s, buffer, strlen( buffer ) );

    irc_read.fd = s;
    irc_read.events[ EVENT_READ ].interested = 1;
    irc_read.events[ EVENT_READ ].callback = irc_handle_incoming;
    irc_read.events[ EVENT_WRITE ].interested = 0;
    irc_read.events[ EVENT_EXCEPT ].interested = 0;

    event_manager_add_event( &irc_read );
}
