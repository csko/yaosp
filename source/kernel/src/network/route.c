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

#include <config.h>

#ifdef ENABLE_NETWORK

#include <errno.h>
#include <console.h>
#include <kernel.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <lock/mutex.h>
#include <network/route.h>
#include <network/socket.h>
#include <lib/array.h>

static lock_id route_mutex;
static array_t static_routes;
static array_t device_routes;

static route_t* route_create( uint8_t* net_addr, uint8_t* net_mask, uint8_t* gateway_addr, uint32_t flags ) {
    route_t* route;

    route = (route_t*)kmalloc( sizeof(route_t) );

    if ( route == NULL ) {
        return NULL;
    }

    memset( route, 0, sizeof( route_t ) );

    IP_COPY_ADDR( route->network_addr, net_addr );
    IP_COPY_ADDR( route->network_mask, net_mask );

    if ( flags & RTF_GATEWAY ) {
        IP_COPY_ADDR( route->gateway_addr, gateway_addr );
    }

    route->flags = flags;
    route->device = NULL;
    route->ref_count = 1;

    return route;
}

static route_t* route_clone( route_t* original ) {
    route_t* clone;

    clone = ( route_t* )kmalloc( sizeof(route_t) );

    if ( clone == NULL ) {
        return NULL;
    }

    clone->ref_count = 1;
    IP_COPY_ADDR( clone->network_addr, original->network_addr );
    IP_COPY_ADDR( clone->network_mask, original->network_mask );
    IP_COPY_ADDR( clone->gateway_addr, original->gateway_addr );

    clone->flags = original->flags;
    clone->device = NULL;

    return clone;
}

static int route_insert( array_t* route_table, route_t* route ) {
    /* TODO: check if the same route already exists. */
    route->ref_count++;
    array_add_item( route_table, route );

    return 0;
}

route_t* route_find( uint8_t* ipv4_addr ) {
    int i;
    int size;
    route_t* route;
    route_t* found;
    net_device_t* device;

    mutex_lock( route_mutex, LOCK_IGNORE_SIGNAL );

    /* Find a device that has a suitable address and netmask to send the packet. */

    device = net_device_get_by_address(ipv4_addr);

    if ( device != NULL ) {
        route = route_create( device->ip_addr, device->netmask, NULL, RTF_UP );

        if ( route != NULL ) {
            route->device = device;
        }

        goto out;
    }

    /* Try to find a suitable static route. */

    size = array_get_size(&static_routes);
    found = NULL;

    for ( i = 0; i < size; i++ ) {
        route_t* tmp;

        tmp = (route_t*)array_get_item( &static_routes, i );

        if ( IP_EQUALS_MASKED( tmp->network_addr, ipv4_addr, tmp->network_mask ) ) {
            found = tmp;
            break;
        }
    }

    /* If nothing is found, or the found
       route is not a gateway then we failed. */

    if ( ( found == NULL ) ||
         ( ( found->flags & RTF_GATEWAY ) == 0 ) ) {
        route = NULL;
        goto out;
    }

    /* Find a network interface for the gateway address. */

    device = net_device_get_by_address(found->gateway_addr);

    if ( device == NULL ) {
        route = NULL;
        goto out;
    }

    /* Clone the original route and assign the network device to it. */

    route = route_clone(found);

    if ( route == NULL ) {
        goto out;
    }

    route->device = device;

 out:
    mutex_unlock( route_mutex );

    return route;
}

route_t* route_find_device( char* dev_name ) {
    int i;
    int size;
    route_t* route;
    net_device_t* device;

    mutex_lock( route_mutex, LOCK_IGNORE_SIGNAL );

    size = array_get_size(&device_routes);

    for ( i = 0; i < size; i++ ) {
        route_t* tmp;

        tmp = (route_t*)array_get_item( &device_routes, i );

        if ( tmp->device == NULL ) {
            kprintf( ERROR, "route_find_device(): found device route without a valid network device!\n" );
            continue;
        }

        if ( strcmp( tmp->device->name, dev_name ) == 0 ) {
            tmp->ref_count++;
            route = tmp;
            goto out;
        }
    }

    device = net_device_get(dev_name);

    if ( device == NULL ) {
        route = NULL;
        goto out;
    }

    route = route_create( device->ip_addr, device->netmask, NULL, RTF_UP );

    if ( route == NULL ) {
        net_device_put(device);
        goto out;
    }

    /* NOTE: we don't have to put the net_device_t later as we assign
             the device to the newly created route entry. */

    route->device = device;

    route_insert( &device_routes, route );

 out:
    mutex_unlock( route_mutex );

    return route;
}

void route_put( route_t* route ) {
    bool do_delete;

    do_delete = false;

    mutex_lock( route_mutex, LOCK_IGNORE_SIGNAL );

    if ( --route->ref_count == 0 ) {
        array_remove_item( &device_routes, route );
        do_delete = true;
    }

    mutex_unlock( route_mutex );

    if ( do_delete ) {
        if ( route->device != NULL ) {
            net_device_put( route->device );
        }

        kfree( route );
    }
}

int route_add( struct rtentry* entry ) {
    int error;
    route_t* route;
    struct sockaddr_in* dst;
    struct sockaddr_in* netmask;
    struct sockaddr_in* gateway;

    dst = (struct sockaddr_in*)&entry->rt_dst;
    netmask = (struct sockaddr_in*)&entry->rt_genmask;
    gateway = (struct sockaddr_in*)&entry->rt_gateway;

    route = route_create( (uint8_t*)&dst->sin_addr, (uint8_t*)&netmask->sin_addr, (uint8_t*)&gateway->sin_addr, entry->rt_flags );

    if ( route == NULL ) {
        return -ENOMEM;
    }

    error = route_insert( &static_routes, route );
    route_put( route );

    return error;
}

int route_get_table( struct rttable* table ) {
    int i;
    int size;
    int count;
    int remaining;
    struct rtabentry* entry;

    entry = (struct rtabentry*)( table + 1 );
    remaining = table->rtt_count;
    table->rtt_count = 0;

    /* Static routes */

    mutex_lock( route_mutex, LOCK_IGNORE_SIGNAL );

    size = array_get_size(&static_routes);

    for ( i = 0; ( i < size ) && ( remaining > 0 ); i++, entry++, remaining--, table->rtt_count++ ) {
        route_t* tmp;
        struct sockaddr_in* addr;

        tmp = (route_t*)array_get_item( &static_routes, i );

        addr = (struct sockaddr_in*)&entry->rt_dst;
        IP_COPY_ADDR( &addr->sin_addr, tmp->network_addr );
        addr = (struct sockaddr_in*)&entry->rt_genmask;
        IP_COPY_ADDR( &addr->sin_addr, tmp->network_mask );
        addr = (struct sockaddr_in*)&entry->rt_gateway;
        IP_COPY_ADDR( &addr->sin_addr, tmp->gateway_addr );
        entry->rt_flags = tmp->flags;

        if ( tmp->device != NULL ) {
            strncpy( entry->rt_dev, tmp->device->name, 64 );
            entry->rt_dev[63] = 0;
        } else {
            entry->rt_dev[0] = 0;
        }
    }

    mutex_unlock( route_mutex );

    /* Network interface routes */

    count = net_device_get_count();

    for ( i = 0;
          ( i < count ) && ( remaining > 0 );
          i++ ) {
        net_device_t* device;
        struct sockaddr_in* addr;

        device = net_device_get_nth(i);

        if ( ( net_device_flags(device) & NETDEV_UP ) == 0 ) {
            net_device_put(device);
            continue;
        }

        entry->rt_flags = RTF_UP;

        addr = (struct sockaddr_in*)&entry->rt_dst;
        IP_COPY_ADDR_MASKED( &addr->sin_addr, device->ip_addr, device->netmask );
        addr = (struct sockaddr_in*)&entry->rt_genmask;
        IP_COPY_ADDR( &addr->sin_addr, device->netmask );

        strncpy( entry->rt_dev, device->name, 64 );
        entry->rt_dev[63] = 0;

        entry++;
        remaining--;
        table->rtt_count++;

        net_device_put(device);
    }

    return 0;
}

__init int init_routes( void ) {
    if ( array_init( &static_routes ) != 0 ) {
        goto error1;
    }

    if ( array_init( &device_routes ) != 0 ) {
        goto error2;
    }

    array_set_realloc_size( &static_routes, 32 );
    array_set_realloc_size( &device_routes, 8 );

    route_mutex = mutex_create( "route mutex", MUTEX_NONE );

    if ( route_mutex < 0 ) {
        goto error3;
    }

    return 0;

 error3:
    array_destroy( &device_routes );

 error2:
    array_destroy( &static_routes );

 error1:
    return -1;
}

#endif /* ENABLE_NETWORK */
