/* ICMP packet handling
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

#ifndef _NETWORK_ICMP_H_
#define _NETWORK_ICMP_H_

#include <types.h>
#include <network/packet.h>

#define ICMP_HEADER_LEN 4

#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO       8

typedef struct icmp_header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
} __attribute__(( packed )) icmp_header_t;

typedef struct icmp_echo {
    uint16_t identifier;
    uint16_t sequence;
} __attribute__(( packed )) icmp_echo_t;

typedef struct icmp_echo_reply {
    uint16_t identifier;
    uint16_t sequence;
} __attribute__(( packed )) icmp_echo_reply_t;

int icmp_input( packet_t* packet );

#endif /* _NETWORK_ICMP_H_ */
