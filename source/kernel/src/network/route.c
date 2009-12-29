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

#include <config.h>

#ifdef ENABLE_NETWORK

#include <errno.h>
#include <console.h>
#include <kernel.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <lock/mutex.h>
#include <network/route.h>
#include <network/interface.h>
#include <lib/string.h>

static lock_id route_mutex;
static hashtable_t route_table;

route_t* create_route( net_interface_t* interface, uint8_t* net_addr, uint8_t* net_mask, uint8_t* gateway_addr, uint32_t flags ) {
    route_t* route;

    route = ( route_t* )kmalloc( sizeof( route_t ) );

    if ( route == NULL ) {
        return NULL;
    }

    memset( route, 0, sizeof( route_t ) );

    atomic_set( &route->ref_count, 1 );
    memcpy( route->network_addr, net_addr, IPV4_ADDR_LEN );
    memcpy( route->network_mask, net_mask, IPV4_ADDR_LEN );

    if ( flags & ROUTE_GATEWAY ) {
        memcpy( route->gateway_addr, gateway_addr, IPV4_ADDR_LEN );
    }

    route->flags = flags;
    route->interface = interface;

    return route;
}

int insert_route( route_t* route ) {
    int error;
    route_t* tmp;

    mutex_lock( route_mutex, LOCK_IGNORE_SIGNAL );

    tmp = ( route_t* )hashtable_get( &route_table, ( const void* )&route->network_addr[ 0 ] );

    if ( tmp != NULL ) {
        kprintf( WARNING, "NET: insert_route(): Route already present!\n" );
        error = -EINVAL;
        goto out;
    }

    hashtable_add( &route_table, ( hashitem_t* )route );

    error = 0;

 out:
    mutex_unlock( route_mutex );

    return error;
}

typedef struct route_iterator_data {
    uint8_t* ipv4_address;
    route_t* route;
} route_iterator_data_t;

static int interface_iterator( hashitem_t* item, void* _data ) {
    net_interface_t* interface;
    route_iterator_data_t* data;

    interface = ( net_interface_t* )item;
    data = ( route_iterator_data_t* )_data;

    if ( ( interface->flags & IFF_UP ) == 0 ) {
        return 0;
    }

    if ( IP_EQUALS_MASKED( interface->ip_address, data->ipv4_address, interface->netmask ) ) {
        route_t* route;

        route = create_route( interface, interface->ip_address, interface->netmask, NULL, 0 );

        if ( route != NULL ) {
            data->route = route;

            return -1;
        }
    }

    return 0;
}

static int route_iterator( hashitem_t* item, void* _data ) {
    route_t* route;
    route_iterator_data_t* data;

    route = ( route_t* )item;
    data = ( route_iterator_data_t* )_data;

    if ( ( IP_EQUALS_MASKED( route->network_addr, data->ipv4_address, route->network_mask ) ) &&
         ( route->flags & ROUTE_GATEWAY ) ) {
        data->route = route;
        return -1;
    }

    return 0;
}

route_t* find_route( uint8_t* ipv4_address ) {
    int ret;
    route_iterator_data_t data;

    data.ipv4_address = ipv4_address;
    data.route = NULL;

    /* Check the interfaces */

    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );
    ret = hashtable_iterate( &interface_table, interface_iterator, &data );
    mutex_unlock( interface_mutex );

    if ( ret != 0 ) {
        ASSERT( data.route != NULL );
        return data.route;
    }

    /* Check the route table */

    data.route = NULL;

    mutex_lock( route_mutex, LOCK_IGNORE_SIGNAL );

    hashtable_iterate( &route_table, route_iterator, ( void* )&data );

    if ( data.route != NULL ) {
        atomic_inc( &data.route->ref_count );
    }

    mutex_unlock( route_mutex );

    return data.route;
}

route_t* find_device_route( const char* device ) {
    route_t* route;
    net_interface_t* interface;

    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );

    interface = ( net_interface_t* )hashtable_get( &interface_table, ( const void* )device );

    if ( interface != NULL ) {
        route = create_route( interface, interface->ip_address, interface->netmask, NULL, 0 );
    } else {
        route = NULL;
    }

    mutex_unlock( interface_mutex );

    return route;
}

void put_route( route_t* route ) {
    bool do_delete;

    do_delete = false;

    mutex_lock( route_mutex, LOCK_IGNORE_SIGNAL );

    if ( atomic_dec_and_test( &route->ref_count ) ) {
        hashtable_remove( &route_table, ( const void* )&route->network_addr[ 0 ] );
        do_delete = true;
    }

    mutex_unlock( route_mutex );

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

__init int init_routes( void ) {
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

    route_mutex = mutex_create( "route mutex", MUTEX_NONE );

    if ( route_mutex < 0 ) {
        destroy_hashtable( &route_table );
        return route_mutex;
    }

    return 0;
}

#endif /* ENABLE_NETWORK */
