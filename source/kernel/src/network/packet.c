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

#include <macros.h>
#include <mm/kmalloc.h>
#include <network/packet.h>

packet_t* create_packet( int size ) {
    uint32_t tmp;
    packet_t* packet;

    packet = ( packet_t* )kmalloc( sizeof( packet_t ) + size + 15 );

    if ( packet == NULL ) {
        return NULL;
    }

    tmp = ( uint32_t )( packet + 1 );
    tmp = ( tmp + 15 ) & ~15;

    packet->data = ( void* )tmp;
    packet->size = size;

    return packet;
}

void delete_packet( packet_t* packet ) {
    kfree( packet );
}

packet_queue_t* create_packet_queue( void ) {
    packet_queue_t* queue;

    queue = ( packet_queue_t* )kmalloc( sizeof( packet_queue_t ) );

    if ( queue == NULL ) {
        goto error1;
    }

    queue->sync = create_semaphore( "packet queue sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( queue->sync < 0 ) {
        goto error2;
    }

    init_spinlock( &queue->lock, "packet queue lock" );

    queue->first = NULL;
    queue->last = NULL;

    return queue;

error2:
    kfree( queue );

error1:
    return NULL;
}

int packet_queue_insert( packet_queue_t* queue, packet_t* packet ) {
    packet->next = NULL;

    spinlock_disable( &queue->lock );

    if ( queue->first == NULL ) {
        ASSERT( queue->last == NULL );

        queue->first = packet;
        queue->last = packet;
    } else {
        ASSERT( queue->last != NULL );

        queue->last->next = packet;
        queue->last = packet;
    }

    spinunlock_enable( &queue->lock );

    UNLOCK( queue->sync );

    return 0;
}
