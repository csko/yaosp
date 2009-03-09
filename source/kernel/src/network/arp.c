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

#include <console.h>
#include <network/arp.h>
#include <network/ethernet.h>
#include <network/device.h>

int arp_input( packet_t* packet ) {
    arp_header_t* arp_header;

    arp_header = ( arp_header_t* )( packet->data + ETH_HLEN );

    kprintf( "ARP command: 0x%x\n", ntohw( arp_header->command ) );

    return 0;
}
