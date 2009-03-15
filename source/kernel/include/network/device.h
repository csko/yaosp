/* Network device definitions
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

#ifndef _NETWORK_DEVICE_H_
#define _NETWORK_DEVICE_H_

#include <types.h>

#define ntohw(n) \
    ((((uint16_t)(n) & 0xFF) << 8) | ((uint16_t)(n) >> 8))
#define ntohl(n) \
    (((uint32_t)(n) << 24) | (((uint32_t)(n) & 0xFF00) << 8) | (((uint32_t)(n) & 0x00FF0000) >> 8) | ((uint32_t)(n) >> 24))

#define htonw ntohw
#define htonl ntohl

typedef struct net_device_stats {
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint32_t rx_errors;
    uint32_t tx_errors;
    uint32_t rx_dropped;
    uint32_t tx_dropped;
} net_device_stats_t;

#endif /* _NETWORK_DEVICE_H_ */
