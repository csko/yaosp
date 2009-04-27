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
#include <time.h>
#include <yaosp/debug.h>

#include "../core/event.h"
#include "../core/eventmanager.h"
#include "../ui/view.h"
#include "../ui/ui.h"
#include "irc.h"
#include "chan.h"

char* my_nick;

static int s;
static event_t irc_read;

static size_t input_size = 0;
static char* input_buffer = NULL;

static const char* timestamp_format = "%D %T";

static void parse_line1(const char* line){
    char tmp[256];
    char* cmd = NULL;
    char* param1 = NULL;
    char* param2 = NULL;

    cmd = strchr( line, ' ' );

    if(cmd != NULL){ /* found command */
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

    snprintf( tmp, sizeof( tmp ), "line1 sender='%s' cmd='%s', param1='%s', param2='%s'", line, cmd, param1, param2 );

    ui_debug_message( tmp );
}

static void parse_line2(const char* line){
    char tmp[256];
    char* params;

    params = strchr( line, ' ' );

    if ( params != NULL ) { /* found params */
        *params++ = '\0';

        if(strcmp(line, "PING") == 0){
            irc_handle_ping( params );

        }
    }

    snprintf(tmp, sizeof( tmp ), "line2 cmd='%s' params='%s'", line, params);

    ui_debug_message( tmp );
}


static void irc_handle_line( char* line ) {
    char tmp[ 256 ];

    snprintf(tmp, sizeof( tmp ), "<< %s", line);
    ui_debug_message( tmp );

    if(line[0] == 0) {
        return;
    }

    if(line[0] == ':') {
        parse_line1(++line);
    } else {
        parse_line2(line);
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

    return irc_write(s, buf, length);
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

int irc_join_channel( const char* channel ) {
    char buf[ 128 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "JOIN %s\r\n", channel );

    irc_write( s, buf, length );

    return 0;
}

int irc_part_channel( const char* channel, const char* message ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "PART %s :%s\r\n", channel, message );

    irc_write( s, buf, length );

    return 0;
}

int irc_send_privmsg( const char* channel, const char* message ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "PRIVMSG %s :%s\r\n", channel, message );

    irc_write( s, buf, length );

    return 0;
}

int irc_raw_command( const char* command ) {
    char buf[ 256 ];
    size_t length;

    length = snprintf( buf, sizeof( buf ), "%s\r\n", command );

    irc_write( s, buf, length );

    return 0;
}

int init_irc( void ) {
    int error;
    char buffer[ 256 ];

    struct sockaddr_in address;

    /* Initialize the channels */

    init_array( &chan_list );

    /* Initialize the connection */
    s = socket( AF_INET, SOCK_STREAM, 0 );

    if ( s < 0 ) {
        return s;
    }

    address.sin_family = AF_INET;
    inet_aton( "157.181.1.129", &address.sin_addr ); /* elte.irc.hu */
    address.sin_port = htons( 6667 );

    error = connect( s, ( struct sockaddr* )&address, sizeof( struct sockaddr_in ) );

    if ( error < 0 ) {
        return error;
    }

    /* TODO: parameterize */
    snprintf( buffer, sizeof( buffer ), "NICK %s\r\nUSER %s SERVER \"elte.irc.hu\" :yaOSp IRC client\r\n", my_nick, my_nick );
    irc_write( s, buffer, strlen( buffer ) );

    /* TODO: handle "nickname already in use" */

    irc_read.fd = s;
    irc_read.events[ EVENT_READ ].interested = 1;
    irc_read.events[ EVENT_READ ].callback = irc_handle_incoming;
    irc_read.events[ EVENT_WRITE ].interested = 0;
    irc_read.events[ EVENT_EXCEPT ].interested = 0;

    event_manager_add_event( &irc_read );

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

    irc_write( s, buf, length );

    return 0;
}

ssize_t irc_write(int fd, const void *buf, size_t count) {
    char tmp[256];

    /* NOTE: size of buf unknown, '\0' is delimiter */
    snprintf(tmp, sizeof( tmp ), ">> %s", ( char* )buf );

    ui_debug_message( tmp );

    return write(fd, buf, count);
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
