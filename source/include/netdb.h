/* yaosp C library
 *
 * Copyright (c) 2009 Zoltan Kovacs
 * Copyright (c) 2010 Kornel Csernai
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

#ifndef _NETDB_H_
#define _NETDB_H_


#include <sys/socket.h> /* socklen_t */

struct hostent {
    char* h_name;
    char** h_aliases;
    int h_addrtype;
    int h_length;
    char** h_addr_list;
#define h_addr h_addr_list[0]
};

struct addrinfo
{
  int ai_flags;             /* Input flags.  */
  int ai_family;            /* Protocol family for socket.  */
  int ai_socktype;          /* Socket type.  */
  int ai_protocol;          /* Protocol for socket.  */
  socklen_t ai_addrlen;     /* Length of socket address.  */
  struct sockaddr *ai_addr; /* Socket address for socket.  */
  char *ai_canonname;       /* Canonical name for service location.  */
  struct addrinfo *ai_next; /* Pointer to next in list.  */
};

/* Description of data base entry for a single service.  */
struct servent
{
  char *s_name;         /* Official service name.  */
  char **s_aliases;     /* Alias list.  */
  int s_port;           /* Port number.  */
  char *s_proto;        /* Protocol to use.  */
};

/* Description of data base entry for a single service.  */
struct protoent
{
  char *p_name;         /* Official protocol name.  */
  char **p_aliases;     /* Alias list.  */
  int p_proto;          /* Protocol number.  */
};

#define HOST_NOT_FOUND  1       /* Authoritative Answer Host not found.  */
#define TRY_AGAIN       2       /* Non-Authoritative Host not found, or SERVERFAIL.  */
#define NO_RECOVERY     3       /* Non recoverable errors, FORMERR, REFUSED, NOTIMP.  */
#define NO_DATA         4       /* Valid name, no data record of requested type.  */

/* TODO: extern? */
int h_errno;

struct hostent* gethostbyname( const char* name );
struct servent* getservbyname( const char* name, const char* proto );

#endif /* _NETDB_H_ */
