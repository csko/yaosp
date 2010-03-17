/* Route handling
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

#ifndef _NETWORK_ROUTE_H_
#define _NETWORK_ROUTE_H_

#include <types.h>
#include <network/ipv4.h>
#include <network/device.h>

#include <arch/atomic.h>

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

typedef struct route {
    int ref_count;

    uint8_t network_addr[ IPV4_ADDR_LEN ];
    uint8_t network_mask[ IPV4_ADDR_LEN ];
    uint8_t gateway_addr[ IPV4_ADDR_LEN ];
    int mask_bits;
    uint32_t flags;

    net_device_t* device;
} route_t;

route_t* route_find( uint8_t* ipv4_address );
route_t* route_find_device( char* name );
void route_put( route_t* route );

int route_add( struct rtentry* entry );
int route_get_table( struct rttable* table );

int init_routes( void );

#endif /* _NETWORK_ROUTE_H_ */
