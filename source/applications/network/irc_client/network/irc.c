/* IRC client
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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
#include <time.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <yaosp/debug.h>

#include "../core/event.h"
#include "../core/eventmanager.h"
#include "../ui/view.h"
#include "../ui/ui.h"
#include "irc.h"
#include "chan.h"

char* my_nick;

static int sock = -1;
static event_t irc_read;
static event_t irc_connect;

static size_t input_size = 0;
static char* input_buffer = NULL;

static const char* timestamp_format = "%H:%M";

static void parse_line1(const char* line){
    char* cmd = NULL;
    char* param1 = NULL;
    char* param2 = NULL;

    cmd = strchr( line, ' ' );

    if ( cmd == NULL ) {
        return;
    }

    *cmd++ = '\0';

    param1 = strchr( cmd, ' ' );

    if(param1 != NULL){ /* found param1 */
        *param1++ = '\0';

        /* Look for a one-parameter command */
        if(strcmp(cmd, "JOIN") == 0){
            irc_handle_join(line, param1);
        }else{ /* Command not found */
            param2 = strchr( param1, ' ' );

            if(param2 != NULL){ /* found param2 */
                *param2++ = '\0';

                /* Look for a two-parameter command */
                if(strcmp(cmd, "PRIVMSG") == 0){
                    irc_handle_privmsg(line, param1, param2 + 1);
                }else if(strcmp(cmd, "NOTICE") == 0){
                    irc_handle_notice(line, param1, param2 + 1);
                }else if(strcmp(cmd, "MODE") == 0){
                    irc_handle_mode(line, param1, param2 + 1);
                }else if(strcmp(cmd, "TOPIC") == 0){
                    irc_handle_topic(line, param1, param2 + 1);
                }else if(strcmp(cmd, "NAMES") == 0){
                    irc_handle_names(line, param1, param2 + 1);
                }else if(strcmp(cmd, "PART") == 0){
                    irc_handle_part(line, param1, param2 + 1);
                }
            }
        }
    }
}

static void parse_line2(const char* line){
    char* params;

    params = strchr( line, ' ' );

    if ( params != NULL ) { /* found params */
        *params++ = '\0';

        if(strcmp(line, "PING") == 0){
            irc_handle_ping( params );

        }
    }
}

static void irc_handle_line( char* line ) {
    switch ( line[0] ) {
        case 0 :
            return;

        case ':' :
            parse_line1(++line);
            break;

        default :
            parse_line2(line);
            break;
    }
}

int irc_handle_privmsg( const char* sender, const char* chan, const char* msg ) {
    char buf[ 256 ];
    view_t* channel;
    struct client _sender;
    int error;

    time_t now;
    char timestamp[ 128 ];

    error = parse_client(sender, &_sender);

    if ( error < 0 ) {
        return error;
    }

    channel = ui_get_channel( chan );

    if ( channel == NULL ) {
        return -1;
    }

    /* Create timestamp */

    time( &now );

    if ( now != ( time_t )-1 ) {
        struct tm tmval;

        gmtime_r( &now, &tmval );
        strftime( ( char* )timestamp, sizeof( timestamp ), timestamp_format, &tmval );
    } else {
        timestamp[ 0 ] = 0;
    }

    snprintf( buf, sizeof( buf ), "%s <%s> %s", timestamp, _sender.nick, msg );

    view_add_text( channel, buf );

    return 0;
}

int irc_handle_notice( const char* sender, const char* chan, const char* msg){
    char buf[ 256 ];
    view_t* channel;
    struct client _sender;
    int error;

    time_t now;
    char timestamp[ 128 ];

    error = parse_client(sender, &_sender);

    if ( error < 0 ) {
        return error;
    }

    channel = ui_get_channel( chan );

    if ( channel == NULL ) {
        return -1;
    }

    /* Create timestamp */

    time( &now );

    if ( now != ( time_t )-1 ) {
        struct tm tmval;

        gmtime_r( &now, &tmval );
        strftime( ( char* )timestamp, sizeof( timestamp ), timestamp_format, &tmval );
    } else {
        timestamp[ 0 ] = 0;
    }

    snprintf( buf, sizeof( buf ), "%s {%s} %s", timestamp, _sender.nick, msg );

    view_add_text( channel, buf );

    return 0;
}

int irc_handle_mode( const char* sender, const char* chan, const char* msg){
    char buf[ 256 ];
    view_t* channel;
    struct client _sender;
    int error;

    time_t now;
    char timestamp[ 128 ];

    error = parse_client(sender, &_sender);

    if ( error < 0 ) {
        return error;
    }

    channel = ui_get_channel( chan );

    if ( channel == NULL ) {
        return -1;
    }

    /* Create timestamp */

    time( &now );

    if ( now != ( time_t )-1 ) {
        struct tm tmval;

        gmtime_r( &now, &tmval );
        strftime( ( char* )timestamp, sizeof( timestamp ), timestamp_format, &tmval );
    } else {
        timestamp[ 0 ] = 0;
    }

    snprintf( buf, sizeof( buf ), "%s {%s} %s", timestamp, _sender.nick, msg );

    view_add_text( channel, buf );

    return 0;
}

int irc_handle_topic( const char* sender, const char* chan, const char* msg){
    return 0;
}

int irc_handle_names( const char* sender, const char* chan, const char* msg){
    return 0;
}

int irc_handle_join( const char* client, const char* chan){
    int error;
    client_t _client;

    error = parse_client(client, &_client);

    if(error != 0){
        return error;
    }

    /* If we join a channel, create a new channel in our internal array */
    if(strcmp(_client.nick, my_nick) == 0){
        error = create_chan(chan);

        if(error != 0){
            return error;
        }

    }

    /* Add new user to the user list of channel */
    addnick_chan(chan, _client.nick, 0);

    return 0;
}

int irc_handle_part( const char* sender, const char* chan, const char* msg){
    return 0;
}

int irc_handle_ping(const char* params){
    char buf[256];
    int length;

    length = snprintf(buf, sizeof(buf), "PONG %s\n", params);

    return irc_write(sock, buf, length);
}

static int irc_handle_incoming( event_t* event ) {
    int size;
    char buffer[ 512 ];

    size = recv( sock, buffer, sizeof( buffer ) - 1, MSG_NOSIGNAL );

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
                input_buffer = NULL;
            } else {
                memmove( input_buffer, start, length );
                input_buffer = ( char* )realloc( input_buffer, length );
            }

            input_size = length;
        }
    }

    return 0;
}

static int irc_handle_connect_finish( event_t* event ) {
    size_t length;
    char buffer[ 256 ];

    event_manager_remove_event( &irc_connect );

    event_init( &irc_read );
    irc_read.fd = sock;
    irc_read.events[ EVENT_READ ].interested = 1;
    irc_read.events[ EVENT_READ ].callback = irc_handle_incoming;

    event_manager_add_event( &irc_read );

    length = snprintf( buffer, sizeof( buffer ),
        "NICK %s\r\nUSER %s SERVER \"elte.irc.hu\" :yaOSp IRC client\r\n", my_nick, my_nick
    );
    irc_write( sock, buffer, length );

    return 0;
}

int irc_connect_to( const char* server ) {
    int ret;
    char buf[32];
    struct hostent* hent;
    struct sockaddr_in addr;

    if ( sock != -1 ) {
        return 0;
    }

    ui_error_message( "Connecting to %s ...\n", server );

    sock = socket( AF_INET, SOCK_STREAM, 0 );

    if ( sock == -1 ) {
        ui_error_message( "Failed to create socket: %s.\n", strerror(errno) );
        return 0;
    }

    /* Set the socket to nonblocking. */

    fcntl( sock, F_SETFL, fcntl( sock, F_GETFL ) | O_NONBLOCK );

    hent = gethostbyname(server);

    if ( hent == NULL ) {
        ui_error_message( "Failed to resolv server address: %s.\n", server );
        return 0;
    }

    addr.sin_family = AF_INET;
    memcpy( &addr.sin_addr, hent->h_addr, hent->h_length );
    addr.sin_port = htons(6667);

    inet_ntop( AF_INET, &addr.sin_addr, buf, sizeof(buf) );
    ui_error_message( "Address of %s is resolved: %s.\n", server, buf );

    ret = connect( sock, (struct sockaddr*)&addr, sizeof(struct sockaddr_in) );

    if ( ret < 0 && errno != EINPROGRESS ) {
        ui_error_message( "Failed to connect to the server: %s.\n", strerror(errno) );
        return 0;
    }

    event_init( &irc_connect );
    irc_connect.fd = sock;
    irc_connect.events[ EVENT_WRITE ].interested = 1;
    irc_connect.events[ EVENT_WRITE ].callback = irc_handle_connect_finish;

    event_manager_add_event( &irc_connect );

    return 0;
}

int irc_join_channel( const char* channel ) {
    char buf[ 128 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "JOIN %s\r\n", channel );
    irc_write( sock, buf, length );

    return 0;
}

int irc_part_channel( const char* channel, const char* message ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "PART %s :%s\r\n", channel, message );
    irc_write( sock, buf, length );

    return 0;
}

int irc_send_privmsg( const char* channel, const char* message ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "PRIVMSG %s :%s\r\n", channel, message );
    irc_write( sock, buf, length );

    return 0;
}

int irc_raw_command( const char* command ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "%s\r\n", command );
    irc_write( sock, buf, length );

    return 0;
}

int init_irc( void ) {
    init_array( &chan_list );

    return 0;
}

int irc_quit_server( const char* reason ) {
    char buf[ 256 ];
    size_t length;

    if ( reason == NULL ) {
        length = snprintf( buf, sizeof( buf ), "QUIT :Leaving...\r\n" );
    } else {
        length = snprintf( buf, sizeof( buf ), "QUIT :%s\r\n", reason );
    }

    irc_write( sock, buf, length );

    return 0;
}

ssize_t irc_write(int fd, const void* buf, size_t count) {
    char* data = (char*)buf;
    size_t remaining = count;

    while ( remaining > 0 ) {
        ssize_t ret = send(fd, data, remaining, MSG_NOSIGNAL);

        if ( ret < 0 ) {
            return ret;
        }

        data += ret;
        remaining -= ret;
    }

    return count;
}

int parse_client( const char* str, client_t* sender ) {
    char* nick;
    char* ident;
    size_t length;

    if ( str == NULL ) {
        return 1;
    }

    nick = strchr( str, '!' );

    if ( nick != NULL ) {
        length = nick - str;

        strncpy( sender->nick, str, length );
        sender->nick[ length ] = 0;

        ident = strchr( nick, '@' );

        if ( ident != NULL ) {
            length = ident - nick;

            strncpy( sender->ident, nick, ident - nick );
            sender->ident[ length ] = 0;

            strcpy( sender->host, ident );
        } else {
            sender->ident[ 0 ] = 0;
            sender->host[ 0 ] = 0;

            return 1;
        }
    } else {
        /* TODO: MAX(strlen(str), sizeof(sender->nick)) ??? */

        strcpy( sender->nick, str );

        return 1;
    }

    return 0;
}
