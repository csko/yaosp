/* yaosp C library
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#ifndef _NET_ROUTE_H_
#define _NET_ROUTE_H_

#include <inttypes.h>

#define RTF_UP      0x0001
#define RTF_GATEWAY 0x0002

struct rtentry {
    unsigned long int rt_pad1;
    struct sockaddr rt_dst;
    struct sockaddr rt_gateway;
    struct sockaddr rt_genmask;
    unsigned short int rt_flags;
    short int rt_pad2;
    unsigned long int rt_pad3;
    unsigned char rt_tos;
    unsigned char rt_class;
    short int rt_pad4;
    short int rt_metric;
    char* rt_dev;
    unsigned long int rt_mtu;
    unsigned long int rt_window;
    unsigned short int rt_irtt;
};

struct rtabentry {
    struct sockaddr rt_dst;
    struct sockaddr rt_gateway;
    struct sockaddr rt_genmask;
    unsigned short int rt_flags;
    uint8_t rt_tos;
    uint8_t rt_class;
    short int rt_metric;
    char rt_dev[64];
    uint32_t rt_mtu;
    uint32_t rt_window;
    uint32_t rt_irtt;
};

struct rttable {
    int rtt_count;
    /* array of struct rtabentry follows */
};

#endif /* _NET_ROUTE_H_ */
