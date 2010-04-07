/* yaosp C library
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <sys/uio.h>

/* Protocol families.  */

#define PF_UNSPEC       0       /* Unspecified.  */
#define PF_LOCAL        1       /* Local to host (pipes and file-domain).  */
#define PF_UNIX         PF_LOCAL /* POSIX name for PF_LOCAL.  */
#define PF_FILE         PF_LOCAL /* Another non-standard name for PF_LOCAL.  */
#define PF_INET         2       /* IP protocol family.  */
#define PF_AX25         3       /* Amateur Radio AX.25.  */
#define PF_IPX          4       /* Novell Internet Protocol.  */
#define PF_APPLETALK    5       /* Appletalk DDP.  */
#define PF_NETROM       6       /* Amateur radio NetROM.  */
#define PF_BRIDGE       7       /* Multiprotocol bridge.  */
#define PF_ATMPVC       8       /* ATM PVCs.  */
#define PF_X25          9       /* Reserved for X.25 project.  */
#define PF_INET6        10      /* IP version 6.  */
#define PF_ROSE         11      /* Amateur Radio X.25 PLP.  */
#define PF_DECnet       12      /* Reserved for DECnet project.  */
#define PF_NETBEUI      13      /* Reserved for 802.2LLC project.  */
#define PF_SECURITY     14      /* Security callback pseudo AF.  */
#define PF_KEY          15      /* PF_KEY key management API.  */
#define PF_NETLINK      16
#define PF_ROUTE        PF_NETLINK /* Alias to emulate 4.4BSD.  */
#define PF_PACKET       17      /* Packet family.  */
#define PF_ASH          18      /* Ash.  */
#define PF_ECONET       19      /* Acorn Econet.  */
#define PF_ATMSVC       20      /* ATM SVCs.  */
#define PF_SNA          22      /* Linux SNA Project */
#define PF_IRDA         23      /* IRDA sockets.  */
#define PF_PPPOX        24      /* PPPoX sockets.  */
#define PF_WANPIPE      25      /* Wanpipe API sockets.  */
#define PF_BLUETOOTH    31      /* Bluetooth sockets.  */
#define PF_IUCV         32      /* IUCV sockets.  */
#define PF_RXRPC        33      /* RxRPC sockets.  */
#define PF_ISDN         34      /* mISDN sockets.  */
#define PF_MAX          35      /* For now..  */

/* Address families. */

#define AF_UNSPEC    PF_UNSPEC
#define AF_LOCAL     PF_LOCAL
#define AF_UNIX      PF_UNIX
#define AF_FILE      PF_FILE
#define AF_INET      PF_INET
#define AF_AX25      PF_AX25
#define AF_IPX       PF_IPX
#define AF_APPLETALK PF_APPLETALK
#define AF_NETROM    PF_NETROM
#define AF_BRIDGE    PF_BRIDGE
#define AF_ATMPVC    PF_ATMPVC
#define AF_X25       PF_X25
#define AF_INET6     PF_INET6
#define AF_ROSE      PF_ROSE
#define AF_DECnet    PF_DECnet
#define AF_NETBEUI   PF_NETBEUI
#define AF_SECURITY  PF_SECURITY
#define AF_KEY       PF_KEY
#define AF_NETLINK   PF_NETLINK
#define AF_ROUTE     PF_ROUTE
#define AF_PACKET    PF_PACKET
#define AF_ASH       PF_ASH
#define AF_ECONET    PF_ECONET
#define AF_ATMSVC    PF_ATMSVC
#define AF_SNA       PF_SNA
#define AF_IRDA      PF_IRDA
#define AF_PPPOX     PF_PPPOX
#define AF_WANPIPE   PF_WANPIPE
#define AF_BLUETOOTH PF_BLUETOOTH
#define AF_IUCV      PF_IUCV
#define AF_RXRPC     PF_RXRPC
#define AF_ISDN      PF_ISDN
#define AF_MAX       PF_MAX

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

/* Types of sockets. */

enum socket_type {
  SOCK_STREAM = 1,              /* Sequenced, reliable, connection-based
                                   byte streams.  */
#define SOCK_STREAM SOCK_STREAM
  SOCK_DGRAM = 2,               /* Connectionless, unreliable datagrams
                                   of fixed maximum length.  */
#define SOCK_DGRAM SOCK_DGRAM
  SOCK_RAW = 3,                 /* Raw protocol interface.  */
#define SOCK_RAW SOCK_RAW
  SOCK_SEQPACKET = 5,           /* Sequenced, reliable, connection-based,
                                   datagrams of fixed maximum length.  */
#define SOCK_SEQPACKET SOCK_SEQPACKET
};

/* Bits in the FLAGS argument to `send', `recv', et al.  */
enum {
    MSG_OOB             = 0x01, /* Process out-of-band data.  */
#define MSG_OOB         MSG_OOB
    MSG_PEEK            = 0x02, /* Peek at incoming messages.  */
#define MSG_PEEK        MSG_PEEK
    MSG_DONTROUTE       = 0x04, /* Don't use local routing.  */
#define MSG_DONTROUTE   MSG_DONTROUTE
#ifdef __USE_GNU
    /* DECnet uses a different name.  */
    MSG_TRYHARD         = MSG_DONTROUTE,
# define MSG_TRYHARD    MSG_DONTROUTE
#endif
    MSG_CTRUNC          = 0x08, /* Control data lost before delivery.  */
#define MSG_CTRUNC      MSG_CTRUNC
    MSG_PROXY           = 0x10, /* Supply or ask second address.  */
#define MSG_PROXY       MSG_PROXY
    MSG_TRUNC           = 0x20,
#define MSG_TRUNC       MSG_TRUNC
    MSG_DONTWAIT        = 0x40, /* Nonblocking IO.  */
#define MSG_DONTWAIT    MSG_DONTWAIT
    MSG_EOR             = 0x80, /* End of record.  */
#define MSG_EOR         MSG_EOR
    MSG_WAITALL         = 0x100, /* Wait for a full request.  */
#define MSG_WAITALL     MSG_WAITALL
    MSG_FIN             = 0x200,
#define MSG_FIN         MSG_FIN
    MSG_SYN             = 0x400,
#define MSG_SYN         MSG_SYN
    MSG_CONFIRM         = 0x800, /* Confirm path validity.  */
#define MSG_CONFIRM     MSG_CONFIRM
    MSG_RST             = 0x1000,
#define MSG_RST         MSG_RST
    MSG_ERRQUEUE        = 0x2000, /* Fetch message from error queue.  */
#define MSG_ERRQUEUE    MSG_ERRQUEUE
    MSG_NOSIGNAL        = 0x4000, /* Do not generate SIGPIPE.  */
#define MSG_NOSIGNAL    MSG_NOSIGNAL
    MSG_MORE            = 0x8000,  /* Sender will send more.  */
#define MSG_MORE        MSG_MORE

    MSG_CMSG_CLOEXEC    = 0x40000000    /* Set close_on_exit for file
                                           descriptor received through
                                           SCM_RIGHTS.  */
#define MSG_CMSG_CLOEXEC MSG_CMSG_CLOEXEC
};

typedef unsigned short int sa_family_t;
typedef unsigned int socklen_t;

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[ 14 ];
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

int socket( int domain, int type, int protocol );
int connect( int fd, const struct sockaddr* address, socklen_t addrlen );
int bind( int sockfd, const struct sockaddr* addr, socklen_t addrlen );
int listen( int sockfd, int backlog );
int accept( int sockfd, struct sockaddr* addr, socklen_t* addrlen );

int getsockopt( int s, int level, int optname,
                void* optval, socklen_t* optlen );
int setsockopt( int s, int level, int optname,
                const void* optval, socklen_t optlen );

int getsockname( int s, struct sockaddr* name, socklen_t* namelen );
int getpeername( int s, struct sockaddr* name, socklen_t* namelen );

ssize_t recv( int s, void* buf, size_t len, int flags );
ssize_t recvfrom( int s, void* buf, size_t len, int flags, struct sockaddr* from, socklen_t* fromlen );
ssize_t recvmsg( int s, struct msghdr* msg, int flags );

ssize_t send( int s, const void* buf, size_t len, int flags );
ssize_t sendto( int s, const void* buf, size_t len, int flags, const struct sockaddr* to, socklen_t tolen );
ssize_t sendmsg( int s, const struct msghdr* msg, int flags );

int shutdown( int sockfd, int how );

/* Not implemented functions
int socketpair( int d, int type, int protocol, int sv[2] );
*/

#endif /* _SYS_SOCKET_H_ */
