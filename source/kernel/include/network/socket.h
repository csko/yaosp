/* Socket handling
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

#ifndef _NETWORK_SOCKET_H_
#define _NETWORK_SOCKET_H_

#include <macros.h>
#include <vfs/inode.h>
#include <vfs/vfs.h>
#include <network/ipv4.h>
#include <lib/hashtable.h>

#define AF_INET 2

#define SOCK_STREAM 1
#define SOCK_DGRAM  2
#define SOCK_RAW    3

#define IPPROTO_TCP 6
#define IPPROTO_UDP 17
#define IPPROTO_RAW 255

/* getsockopt() and setsockopt() constants */

#define SOL_SOCKET 1

#define SO_DEBUG        1
#define SO_REUSEADDR    2
#define SO_TYPE         3
#define SO_ERROR        4
#define SO_DONTROUTE    5
#define SO_BROADCAST    6
#define SO_SNDBUF       7
#define SO_RCVBUF       8
#define SO_SNDBUFFORCE  32
#define SO_RCVBUFFORCE  33
#define SO_KEEPALIVE    9
#define SO_OOBINLINE    10
#define SO_NO_CHECK     11
#define SO_PRIORITY     12
#define SO_LINGER       13
#define SO_BSDCOMPAT    14
#define SO_REUSEPORT    15
#define SO_PASSCRED     16
#define SO_PEERCRED     17
#define SO_RCVLOWAT     18
#define SO_SNDLOWAT     19
#define SO_RCVTIMEO     20
#define SO_SNDTIMEO     21
#define SO_BINDTODEVICE 22

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

struct msghdr {
    void* msg_name;
    socklen_t msg_namelen;
    struct iovec* msg_iov;
    size_t msg_iovlen;
    void* msg_control;
    socklen_t msg_controllen;
    int msg_flags;
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

    int so_error;

    void* data;
    struct socket_calls* operations;
} __PACKED socket_t;

struct select_request;

typedef struct socket_calls {
    int ( *close )( socket_t* socket );
    int ( *connect )( socket_t* socket, struct sockaddr* address, socklen_t addrlen );
    int ( *bind )( socket_t* socket, struct sockaddr* address, socklen_t addrlen );
    int ( *recvmsg )( socket_t* socket, struct msghdr* msg, int flags );
    int ( *sendmsg )( socket_t* socket, struct msghdr* msg, int flags );
    int ( *getsockopt )( socket_t* socket, int level, int optname, void* optval, socklen_t* optlen );
    int ( *setsockopt )( socket_t* socket, int level, int optname, void* optval, socklen_t optlen );
    int ( *set_flags )( socket_t* socket, int flags );
    int ( *add_select_request )( socket_t* socket, struct select_request* request );
    int ( *remove_select_request )( socket_t* socket, struct select_request* request );
} socket_calls_t;

int socket_get_error( socket_t* socket );

int socket_set_error( socket_t* socket, int error );

int sys_socket( int family, int type, int protocol );
int sys_connect( int fd, struct sockaddr* address, socklen_t addrlen );
int sys_bind( int sockfd, struct sockaddr* addr, socklen_t addrlen );
int sys_listen( int sockfd, int backlog );
int sys_accept( int sockfd, struct sockaddr* addr, socklen_t* addrlen );
int sys_getsockopt( int s, int level, int optname, void* optval, socklen_t* optlen );
int sys_setsockopt( int s, int level, int optname, void* optval, socklen_t optlen );
int sys_getsockname( int s, struct sockaddr* name, socklen_t* namelen );
int sys_getpeername( int s, struct sockaddr* name, socklen_t* namelen );
int sys_recvmsg( int fd, struct msghdr* msg, int flags );
int sys_sendmsg( int fd, struct msghdr* msg, int flags );

int init_socket( void );

#endif /* _NETWORK_SOCKET_H_ */
