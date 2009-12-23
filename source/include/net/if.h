/* yaosp C library
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

#ifndef _NET_IF_H_
#define _NET_IF_H_

#define IFF_UP ( 1 << 0 )

#define IFHWADDRLEN 6
#define IFNAMSIZ    16

struct ifreq {
    union {
        char ifrn_name[ IFNAMSIZ ];
    } ifr_ifrn;

    union {
        struct sockaddr ifru_addr;
        struct sockaddr ifru_dstaddr;
        struct sockaddr ifru_broadaddr;
        struct sockaddr ifru_netmask;
        struct sockaddr ifru_hwaddr;
        short ifru_flags;

        int	ifru_ivalue;
        int	ifru_mtu;
        char ifru_slave[ IFNAMSIZ ];
        char ifru_newname[ IFNAMSIZ ];
        char* ifru_data;
    } ifr_ifru;
};

struct ifconf {
    int	ifc_len;
    union  {
        char* ifcu_buf;
        struct ifreq* ifcu_req;
    } ifc_ifcu;
};

#endif /* _NET_IF_H_ */
