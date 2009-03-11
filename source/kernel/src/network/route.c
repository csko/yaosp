/* Route handling
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

#include <semaphore.h>
#include <errno.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <network/route.h>
#include <lib/string.h>

typedef struct route_iterator_data {
    uint8_t* ipv4_address;
    route_t* route;
} route_iterator_data_t;

static hashtable_t route_table;
static semaphore_id route_lock;

route_t* create_route( uint8_t* net_addr, uint8_t* net_mask, uint8_t* gateway_addr, uint32_t flags ) {
    route_t* route;

    route = ( route_t* )kmalloc( sizeof( route_t ) );

    if ( route == NULL ) {
        return NULL;
    }

    atomic_set( &route->ref_count, 1 );
    memcpy( route->network_addr, net_addr, IPV4_ADDR_LEN );
    memcpy( route->network_mask, net_mask, IPV4_ADDR_LEN );
    memcpy( route->gateway_addr, gateway_addr, IPV4_ADDR_LEN );
    route->flags = flags;

    return route;
}

int insert_route( route_t* route ) {
    int error;
    route_t* tmp;

    LOCK( route_lock );

    tmp = ( route_t* )hashtable_get( &route_table, ( const void* )&route->network_addr[ 0 ] );

    if ( tmp != NULL ) {
        kprintf( "NET: insert_route(): Route already present!\n" );
        error = -EINVAL;
        goto out;
    }

    hashtable_add( &route_table, ( hashitem_t* )route );

    error = 0;

out:
    UNLOCK( route_lock );

    return error;
}

static int route_iterator( hashitem_t* item, void* _data ) {
    route_t* route;
    uint32_t route_ip;
    uint32_t dest_ip;
    uint32_t net_mask;
    route_iterator_data_t* data;

    route = ( route_t* )item;
    data = ( route_iterator_data_t* )_data;

    route_ip = *( ( uint32_t* )route->network_addr );
    dest_ip = *( ( uint32_t* )data->ipv4_address );
    net_mask = *( ( uint32_t* )route->network_mask );

    if ( ( route_ip & net_mask ) == ( dest_ip & net_mask ) ) {
        data->route = route;
        return -1;
    }

    return 0;
}

route_t* find_route( uint8_t* ipv4_address ) {
    route_iterator_data_t data;

    data.ipv4_address = ipv4_address;
    data.route = NULL;

    LOCK( route_lock );

    hashtable_iterate( &route_table, route_iterator, ( void* )&data );

    if ( data.route != NULL ) {
        atomic_inc( &data.route->ref_count );
    }

    UNLOCK( route_lock );

    return data.route;
}

void put_route( route_t* route ) {
    bool do_delete;

    do_delete = false;

    LOCK( route_lock );

    if ( atomic_dec_and_test( &route->ref_count ) ) {
        hashtable_remove( &route_table, ( const void* )&route->network_addr[ 0 ] );
        do_delete = true;
    }

    UNLOCK( route_lock );

    if ( do_delete ) {
        /* TODO: put the interface */
        kfree( route );
    }
}

static void* route_key( hashitem_t* item ) {
    route_t* route;

    route = ( route_t* )item;

    return &route->network_addr[ 0 ];
}

static uint32_t route_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, 3 * IPV4_ADDR_LEN );
}

static bool route_compare( const void* key1, const void* key2 ) {
    return ( memcmp( key1, key2, 3 * IPV4_ADDR_LEN ) == 0 );
}

int init_routes( void ) {
    int error;

    error = init_hashtable(
        &route_table,
        32,
        route_key,
        route_hash,
        route_compare
    );

    if ( error < 0 ) {
        return error;
    }

    route_lock = create_semaphore( "route lock", SEMAPHORE_BINARY, 0, 1 );

    if ( route_lock < 0 ) {
        destroy_hashtable( &route_table );
        return route_lock;
    }

    return 0;
}
