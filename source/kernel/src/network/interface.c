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

#include <kernel.h>
#include <errno.h>
#include <console.h>
#include <ioctl.h>
#include <lock/mutex.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <network/interface.h>
#include <network/ethernet.h>
#include <network/device.h>
#include <network/arp.h>
#include <network/ipv4.h>
#include <network/route.h>
#include <lib/string.h>

static lock_id interface_mutex;
static uint32_t interface_counter = 0;
static hashtable_t interface_table;

static net_interface_t* alloc_network_interface( void ) {
    net_interface_t* interface;

    interface = ( net_interface_t* )kmalloc( sizeof( net_interface_t ) );

    if ( interface == NULL ) {
        goto error1;
    }

    memset( interface, 0, sizeof( net_interface_t ) );

    interface->input_queue = create_packet_queue();

    if ( interface->input_queue == NULL ) {
        kfree( interface );
        goto error2;
    }

    interface->device = -1;

    return interface;

error2:
    kfree( interface );

error1:
    return NULL;
}

static int insert_network_interface( net_interface_t* interface ) {
    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );

    atomic_set( &interface->ref_count, 1 );

    do {
        snprintf( interface->name, sizeof( interface->name ), "eth%u", interface_counter );
        interface_counter++;
    } while ( hashtable_get( &interface_table, ( const void* )interface->name ) != NULL );

    hashtable_add( &interface_table, ( hashitem_t* )interface );

    mutex_unlock( interface_mutex );

    return 0;
}

int ethernet_send_packet( net_interface_t* interface, uint8_t* hw_address, uint16_t protocol, packet_t* packet ) {
    int error;
    ethernet_header_t* eth_header;

    eth_header = ( ethernet_header_t* )packet->data;

    /* Build the ethernet header */

    eth_header->proto = htonw( protocol );

    memcpy( eth_header->dest, hw_address, ETH_ADDR_LEN );
    memcpy( eth_header->src, interface->hw_address, ETH_ADDR_LEN );

    /* Send the packet */

    error = pwrite( interface->device, packet->data, packet->size, 0 );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int network_rx_thread( void* data ) {
    packet_t* packet;
    net_interface_t* interface;
    ethernet_header_t* eth_header;

    interface = ( net_interface_t* )data;

    while ( 1 ) {
        packet = packet_queue_pop_head( interface->input_queue, INFINITE_TIMEOUT );

        if ( packet == NULL ) {
            continue;
        }

        eth_header = ( ethernet_header_t* )packet->data;
        packet->network_data = ( uint8_t* )( eth_header + 1 );

        switch ( ntohw( eth_header->proto ) ) {
            case ETH_P_IP :
                ipv4_input( packet );
                break;

            case ETH_P_ARP :
                arp_input( interface, packet );
                break;

            default :
                kprintf( WARNING, "NET: Unknown protocol: %x\n", ntohw( eth_header->proto ) );
                break;
        }
    }

    return 0;
}

static int get_interface_count( void ) {
    int count;

    mutex_lock( interface_mutex, LOCK_IGNORE_SIGNAL );

    count = hashtable_get_item_count( &interface_table );

    mutex_unlock( interface_mutex );

    return count;
}

static int start_network_interface( net_interface_t* interface ) {
    int error;

    interface->rx_thread = create_kernel_thread(
        "network_rx",
        PRIORITY_NORMAL,
        network_rx_thread,
        ( void* )interface,
        0
    );

    if ( interface->rx_thread < 0 ) {
        return -1;
    }

    thread_wake_up( interface->rx_thread );

    /* TODO */
    uint8_t netmask[4]={255,255,255,0};
    uint8_t dummy[4]={0,0,0,0};
    route_t* route;
    interface->ip_address[ 0 ] = 192;
    interface->ip_address[ 1 ] = 168;
    interface->ip_address[ 2 ] = 1;
    interface->ip_address[ 3 ] = 192;
    uint8_t gateway[4]={192,168,1,1};
    route = create_route( interface->ip_address, netmask, dummy, 0 );
    route->interface = interface;
    insert_route( route );
    dummy[1]=1;
    route = create_route( interface->ip_address, netmask, gateway, ROUTE_GATEWAY );
    route->interface = interface;
    insert_route( route );
    /* End of TODO */

    error = ioctl( interface->device, IOCTL_NET_GET_HW_ADDRESS, ( void* )interface->hw_address );

    if ( error < 0 ) {
        return error;
    }

    error = ioctl( interface->device, IOCTL_NET_SET_IN_QUEUE, ( void* )interface->input_queue );

    if ( error < 0 ) {
        return error;
    }

    error = ioctl( interface->device, IOCTL_NET_START, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int create_network_interface( int device ) {
    net_interface_t* interface;

    interface = alloc_network_interface();

    if ( interface == NULL ) {
        return -ENOMEM;
    }

    interface->mtu = 1500;
    interface->device = device;

    insert_network_interface( interface );
    start_network_interface( interface );

    kprintf( INFO, "Created network interface: %s\n", interface->name );

    return 0;
}

int network_interface_ioctl( int command, void* buffer, bool from_kernel ) {
    int error;

    switch ( command ) {
        case SIOCGIFCOUNT :
            error = get_interface_count();
            break;

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

__init int create_network_interfaces( void ) {
    int dir;
    int device;
    dirent_t entry;
    char path[ 128 ];

    dir = open( "/device/network", O_RDONLY );

    if ( dir < 0 ) {
        return dir;
    }

    while ( getdents( dir, &entry, sizeof( dirent_t ) ) == 1 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        snprintf( path, sizeof( path ), "/device/network/%s", entry.name );

        device = open( path, O_RDWR );

        if ( device < 0 ) {
            continue;
        }

        create_network_interface( device );
    }

    close( dir );

    return 0;
}

static void* net_if_key( hashitem_t* item ) {
    net_interface_t* interface;

    interface = ( net_interface_t* )item;

    return ( void* )interface->name;
}

__init int init_network_interfaces( void ) {
    int error;

    /* Initialize network interface table */

    error = init_hashtable(
        &interface_table,
        32,
        net_if_key,
        hash_str,
        compare_str
    );

    if ( error < 0 ) {
        goto error1;
    }

    interface_mutex = mutex_create( "network interface mutex", MUTEX_NONE );

    if ( interface_mutex < 0 ) {
        error = interface_mutex;
        goto error2;
    }

    return 0;

error2:
    destroy_hashtable( &interface_table );

error1:
    return error;
}
