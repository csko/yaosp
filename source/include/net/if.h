/* yaosp C library
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

#ifndef _NET_IF_H_
#define _NET_IF_H_

#define IFF_UP ( 1 << 2 )

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

#define ifr_name       ifr_ifrn.ifrn_name      /* interface name       */
#define ifr_hwaddr     ifr_ifru.ifru_hwaddr    /* MAC address          */
#define ifr_addr       ifr_ifru.ifru_addr      /* address              */
#define ifr_dstaddr    ifr_ifru.ifru_dstaddr   /* other end of p-p lnk */
#define ifr_broadaddr  ifr_ifru.ifru_broadaddr /* broadcast address    */
#define ifr_netmask    ifr_ifru.ifru_netmask   /* interface net mask   */
#define ifr_flags      ifr_ifru.ifru_flags     /* flags                */
#define ifr_metric     ifr_ifru.ifru_ivalue    /* metric               */
#define ifr_mtu        ifr_ifru.ifru_mtu       /* mtu                  */
};

struct ifconf {
    int	ifc_len;
    union  {
        char* ifcu_buf;
        struct ifreq* ifcu_req;
    } ifc_ifcu;
};

#endif /* _NET_IF_H_ */
