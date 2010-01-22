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

#include <config.h>

#ifdef ENABLE_NETWORK

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
    packet->network_data = NULL;
    packet->transport_data = NULL;
    packet->size = size;

    return packet;
}

void delete_packet( packet_t* packet ) {
    kfree( packet );
}

int packet_queue_init( packet_queue_t* queue ) {
    queue->sync = semaphore_create( "packet queue semaphore", 0 );

    if ( queue->sync < 0 ) {
        return -1;
    }

    init_spinlock( &queue->lock, "packet queue lock" );

    queue->first = NULL;
    queue->last = NULL;

    return 0;
}

int packet_queue_destroy( packet_queue_t* queue ) {
    packet_t* packet;

    while ( queue->first != NULL ) {
        packet = queue->first;
        queue->first = packet->next;

        delete_packet( packet );
    }

    semaphore_destroy( queue->sync );

    return 0;
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

    semaphore_unlock( queue->sync, 1 );

    return 0;
}

packet_t* packet_queue_pop_head( packet_queue_t* queue, uint64_t timeout ) {
    packet_t* packet;

    if ( semaphore_timedlock( queue->sync, 1, LOCK_IGNORE_SIGNAL, timeout ) != 0 ) {
        return NULL;
    }

    spinlock_disable( &queue->lock );

    packet = queue->first;

    if ( packet != NULL ) {
        queue->first = packet->next;

        if ( queue->first == NULL ) {
            queue->last = NULL;
        }
    }

    spinunlock_enable( &queue->lock );

    return packet;
}

int packet_queue_is_empty( packet_queue_t* queue ) {
    int ret;

    spinlock_disable( &queue->lock );
    ret = ( queue->first == NULL );
    spinunlock_enable( &queue->lock );

    return ret;
}

#endif /* ENABLE_NETWORK */
