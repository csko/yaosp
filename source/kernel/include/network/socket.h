/* Socket handling
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

#ifndef _NETWORK_SOCKET_H_
#define _NETWORK_SOCKET_H_

#include <vfs/inode.h>
#include <network/ipv4.h>
#include <lib/hashtable.h>

#define AF_INET 2

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_RAW 255

typedef uint16_t sa_family_t;
typedef uint32_t socklen_t;
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[ 14 ];
};

struct sockaddr_in {
    sa_family_t sin_family;
    in_port_t sin_port;      /* Port number.  */
    struct in_addr sin_addr; /* Internet address.  */

    /* Pad to size of `struct sockaddr'. */
    unsigned char sin_zero[
        sizeof( struct sockaddr ) -
        sizeof( sa_family_t ) -
        sizeof( in_port_t ) -
        sizeof( struct in_addr )
    ];
};

struct socket_calls;

typedef struct socket {
    hashitem_t hash;

    int family;
    int type;

    ino_t inode_number;

    uint8_t src_address[ IPV4_ADDR_LEN ];
    uint16_t src_port;
    uint8_t dest_address[ IPV4_ADDR_LEN ];
    uint16_t dest_port;

    void* data;
    struct socket_calls* operations;
} __attribute__(( packed )) socket_t;

typedef struct socket_calls {
    int ( *close )( socket_t* socket );
    int ( *connect )( socket_t* socket, struct sockaddr* address, socklen_t addrlen );
    int ( *read )( socket_t* socket, void* data, size_t length );
    int ( *write )( socket_t* socket, const void* data, size_t length );
    int ( *set_flags )( socket_t* socket, int flags );
    int ( *add_select_request )( socket_t* socket, struct select_request* request );
    int ( *remove_select_request )( socket_t* socket, struct select_request* request );
} socket_calls_t;

int sys_socket( int family, int type, int protocol );
int sys_connect( int fd, struct sockaddr* address, socklen_t addrlen );

int init_socket( void );

#endif /* _NETWORK_SOCKET_H_ */
