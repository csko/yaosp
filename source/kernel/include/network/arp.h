/* Network interface handling
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

#ifndef _NETWORK_ARP_H_
#define _NETWORK_ARP_H_

#include <types.h>
#include <network/packet.h>

typedef struct arp_header {
    uint16_t hardware_addr_format;
    uint16_t protocol_addr_format;
    uint8_t hardware_addr_size;
    uint8_t protocol_addr_size;
    uint16_t command;
} arp_header_t;

int arp_input( packet_t* packet );

#endif /* _NETWORK_ARP_H_ */
