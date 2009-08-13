/* Network packet handling
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

#ifndef _NETWORK_PACKET_H_
#define _NETWORK_PACKET_H_

#include <types.h>
#include <lock/semaphore.h>

#include <arch/spinlock.h>

typedef struct packet {
    struct packet* next;

    uint8_t* data;
    uint8_t* network_data;
    uint8_t* transport_data;

    int size;
} packet_t;

typedef struct packet_queue {
    spinlock_t lock;
    lock_id sync;
    packet_t* first;
    packet_t* last;
} packet_queue_t;

packet_t* create_packet( int size );
void delete_packet( packet_t* packet );

packet_queue_t* create_packet_queue( void );
void delete_packet_queue( packet_queue_t* packet_queue );
int packet_queue_insert( packet_queue_t* queue, packet_t* packet );
packet_t* packet_queue_pop_head( packet_queue_t* queue, uint64_t timeout );

#endif /* _NETWORK_PACKET_H_ */
