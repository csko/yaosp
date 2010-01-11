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

#ifndef _NETWORK_IRC_H_
#define _NETWORK_IRC_H_
#include <unistd.h>

#include "yutil/array.h"

array_t chan_list; /* chan_t* */

typedef struct client {
    /* TODO: better lengths */
    char nick[32];
    char ident[32];
    char host[256];
} client_t;

/* User commands */
int irc_join_channel( const char* channel );
int irc_part_channel( const char* channel, const char* message );
int irc_send_privmsg( const char* channel, const char* message );
int irc_raw_command( const char* channel );
int irc_quit_server( const char* reason );

/* Handlers */
/* one-parameter */
int irc_handle_ping( const char* msg);
int irc_handle_join( const char* client, const char* chan);

/* two-parameter */
int irc_handle_privmsg( const char* sender, const char* chan, const char* msg);
int irc_handle_notice( const char* sender, const char* chan, const char* msg);
int irc_handle_mode( const char* sender, const char* chan, const char* msg);
int irc_handle_topic( const char* sender, const char* chan, const char* msg);
int irc_handle_names( const char* sender, const char* chan, const char* msg);
int irc_handle_part( const char* sender, const char* chan, const char* msg);

int init_irc( void );

/* Wrapper for write() */
ssize_t irc_write(int fd, const void *buf, size_t count);

int parse_client(const char* str, client_t* sender);

#endif /* _NETWORK_IRC_H_ */
