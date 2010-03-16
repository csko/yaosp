/* Network device handling
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <ioctl.h>
#include <errno.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <network/device.h>
#include <network/interface.h>
#include <network/socket.h>
#include <network/arp.h>
#include <network/route.h>
#include <lib/array.h>

static uint32_t device_id;
static lock_id device_lock;
static array_t device_table;

net_device_t* net_device_create( size_t priv_size ) {
    net_device_t* device;

    device = ( net_device_t* )kmalloc( sizeof( net_device_t ) + priv_size );

    if ( device == NULL ) {
        goto error1;
    }

    memset( device, 0, sizeof( net_device_t ) );

    device->lock = mutex_create( "net dev lock", MUTEX_NONE ); /* todo */

    if ( device->lock < 0 ) {
        goto error2;
    }

    device->ref_count = 1;
    device->private = ( void* )( device + 1 );
    device->mtu = 1500; /* todo */

    packet_queue_init( &device->input_queue );
    arp_interface_init( device );

    return device;

 error2:
    kfree( device );

 error1:
    return NULL;
}

int net_device_free( net_device_t* device ) {
    kfree( device );

    return 0;
}

static net_device_t* do_net_device_get( char* name ) {
    int i;
    int size;

    size = array_get_size( &device_table );

    for ( i = 0; i < size; i++ ) {
        net_device_t* device;

        device = ( net_device_t* )array_get_item( &device_table, i );

        if ( strcmp( device->name, name ) == 0 ) {
            return device;
        }
    }

    return NULL;
}

int net_device_register( net_device_t* device ) {
    int error;

    mutex_lock( device_lock, LOCK_IGNORE_SIGNAL );

    do {
        snprintf( device->name, sizeof( device->name ), "eth%u", device_id++ );
    } while ( do_net_device_get( device->name ) != NULL );

    error = array_add_item( &device_table, ( void* )device );

    mutex_unlock( device_lock );

    return error;
}

net_device_t* net_device_get( char* name ) {
    net_device_t* device;

    mutex_lock( device_lock, LOCK_IGNORE_SIGNAL );

    device = do_net_device_get( name );

    if ( device != NULL ) {
        device->ref_count++;
    }

    mutex_unlock( device_lock );

    return device;
}

net_device_t* net_device_get_nth( int index ) {
    net_device_t* device;

    mutex_lock( device_lock, LOCK_IGNORE_SIGNAL );

    if ( ( index < 0 ) ||
         ( index >= array_get_size( &device_table ) ) ) {
        device = NULL;
    } else {
        device = ( net_device_t* )array_get_item( &device_table, index );
        device->ref_count++;
    }

    mutex_unlock( device_lock );

    return device;
}

net_device_t* net_device_get_by_address( uint8_t* ipv4_addr ) {
    int i;
    int size;
    net_device_t* device = NULL;

    mutex_lock( device_lock, LOCK_IGNORE_SIGNAL );

    size = array_get_size(&device_table);

    for ( i = 0; i < size; i++ ) {
        net_device_t* tmp;

        tmp = (net_device_t*)array_get_item( &device_table, i );

        if ( IP_EQUALS_MASKED( tmp->ip_addr, ipv4_addr, tmp->netmask ) ) {
            device = tmp;
            device->ref_count++;
            break;
        }
    }

    mutex_unlock( device_lock );

    return device;
}

int net_device_get_count( void ) {
    int size;

    mutex_lock( device_lock, LOCK_IGNORE_SIGNAL );
    size = array_get_size( &device_table );
    mutex_unlock( device_lock );

    return size;
}

int net_device_put( net_device_t* device ) {
    int do_delete = 0;

    mutex_lock( device_lock, LOCK_IGNORE_SIGNAL );

    if ( --device->ref_count == 0 ) {
        array_remove_item( &device_table, device );
        do_delete = 1;
    }

    mutex_unlock( device_lock );

    if ( do_delete ) {
        /* todo */
    }

    return 0;
}

int net_device_flags( net_device_t* device ) {
    return atomic_get( &device->flags );
}

int net_device_running( net_device_t* device ) {
    return ( atomic_get( &device->flags ) & NETDEV_RUNNING );
}

int net_device_carrier_ok( net_device_t* device ) {
    return ( atomic_get( &device->flags ) & NETDEV_CARRIER_ON );
}

int net_device_carrier_on( net_device_t* device ) {
    atomic_or( &device->flags, NETDEV_CARRIER_ON );
    return 0;
}

int net_device_carrier_off( net_device_t* device ) {
    atomic_and( &device->flags, ~NETDEV_CARRIER_ON );
    return 0;
}

int net_device_insert_packet( net_device_t* device, packet_t* packet ) {
    return packet_queue_insert( &device->input_queue, packet );
}

int net_device_transmit( net_device_t* device, packet_t* packet ) {
    if ( device->ops->transmit( device, packet ) != NETDEV_TX_OK ) {
        return -1;
    }

    return 0;
}

void* net_device_get_private( net_device_t* device ) {
    return device->private;
}

int net_device_init( void ) {
    if ( array_init( &device_table ) != 0 ) {
        return -1;
    }

    array_set_realloc_size( &device_table, 32 );

    device_lock = mutex_create( "netdev table lock", MUTEX_NONE );

    if ( device_lock < 0 ) {
        return -1;
    }

    device_id = 0;

    return 0;
}

static int network_rx_thread( void* data ) {
    int error;
    net_device_t* device;

    device = ( net_device_t* )data;

    /* Start the network device */

    error = device->ops->open( device );

    if ( error < 0 ) {
        return 0;
    }

    atomic_or( &device->flags, NETDEV_RUNNING );

    while ( 1 ) {
        packet_t* packet;

        /* Get the next packet from the input queue */

        packet = packet_queue_pop_head( &device->input_queue, INFINITE_TIMEOUT );

        if ( packet == NULL ) {
            continue;
        }

        /* If the interface is not UP, the packet will be discarded simply. */

        if ( ( net_device_flags( device ) & NETDEV_UP ) == 0 ) {
            delete_packet( packet );
            continue;
        }

        /* Process the incoming packet */

        ethernet_input( device, packet );
    }

    return 0;
}

static int do_net_device_start( net_device_t* device ) {
    /* Create the RX thread for the device. */

    device->rx_thread = create_kernel_thread(
        "network_rx",
        PRIORITY_NORMAL,
        network_rx_thread,
        ( void* )device,
        0
    );

    if ( device->rx_thread < 0 ) {
        return 0;
    }

    /* Start the RX thread. */

    thread_wake_up( device->rx_thread );

    return 0;
}

int net_device_start( void ) {
    int i;
    int size;

    size = array_get_size( &device_table );

    for ( i = 0; i < size; i++ ) {
        net_device_t* device;

        device = ( net_device_t* )array_get_item( &device_table, i );

        do_net_device_start( device );
    }

    return 0;
}

static int get_interface_list( struct ifconf* list ) {
    int i;
    int size;

    mutex_lock( device_lock, LOCK_IGNORE_SIGNAL );

    size = array_get_size( &device_table );
    size = MIN( size, list->ifc_len );

    for ( i = 0; i < size; i++ ) {
        struct ifreq* req;
        net_device_t* device;

        req = &list->ifc_ifcu.ifcu_req[ i ];
        device = ( net_device_t* )array_get_item( &device_table, i );

        strncpy( req->ifr_ifrn.ifrn_name, device->name, IFNAMSIZ );
        req->ifr_ifrn.ifrn_name[ IFNAMSIZ - 1 ] = 0;
    }

    mutex_unlock( device_lock );

    return 0;
}

static int get_interface_parameter( int param, struct ifreq* req ) {
    net_device_t* device;
    struct sockaddr_in* addr;

    device = net_device_get( req->ifr_ifrn.ifrn_name );

    if ( device == NULL ) {
        return -EINVAL;
    }

    mutex_lock( device->lock, LOCK_IGNORE_SIGNAL );

    switch ( param ) {
        case SIOCGIFADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_addr;
            memcpy( &addr->sin_addr, device->ip_addr, IPV4_ADDR_LEN );
            break;

        case SIOCGIFNETMASK :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_netmask;
            memcpy( &addr->sin_addr, device->netmask, IPV4_ADDR_LEN );
            break;

        case SIOCGIFBRDADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_broadaddr;
            memcpy( &addr->sin_addr, device->broadcast, IPV4_ADDR_LEN );
            break;

        case SIOCGIFHWADDR :
            memcpy( req->ifr_ifru.ifru_hwaddr.sa_data, device->dev_addr, ETH_ADDR_LEN );
            break;

        case SIOCGIFFLAGS :
            req->ifr_ifru.ifru_flags = atomic_get( &device->flags );
            break;

        case SIOCGIFMTU :
            req->ifr_ifru.ifru_mtu = device->mtu;
            break;

        case SIOCGIFSTAT :
            memcpy( req->ifr_ifru.ifru_data, &device->stats, sizeof( net_device_stats_t ) );
            break;

        default :
            kprintf( ERROR, "get_interface_parameter(): invalid request: %d\n", param );
            break;
    }

    mutex_unlock( device->lock );

    net_device_put( device );

    return 0;
}

static int set_interface_parameter( int param, struct ifreq* req ) {
    net_device_t* device;
    struct sockaddr_in* addr;

    device = net_device_get( req->ifr_ifrn.ifrn_name );

    if ( device == NULL ) {
        return -EINVAL;
    }

    mutex_lock( device->lock, LOCK_IGNORE_SIGNAL );

    switch ( param ) {
        case SIOCSIFADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_addr;
            memcpy( device->ip_addr, &addr->sin_addr, IPV4_ADDR_LEN );
            break;

        case SIOCSIFNETMASK :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_netmask;
            memcpy( device->netmask, &addr->sin_addr, IPV4_ADDR_LEN );
            break;

        case SIOCSIFBRDADDR :
            addr = ( struct sockaddr_in* )&req->ifr_ifru.ifru_broadaddr;
            memcpy( device->broadcast, &addr->sin_addr, IPV4_ADDR_LEN );
            break;

        case SIOCSIFFLAGS :
            /* todo: this is not really atomic ;) */
            atomic_set(
                &device->flags,
                ( atomic_get( &device->flags ) & ~NETDEV_USER_MASK ) | ( req->ifr_ifru.ifru_flags & NETDEV_USER_MASK )
            );

            break;

        default :
            kprintf( ERROR, "set_interface_parameter(): invalid request: %d\n", param );
            break;
    }

    mutex_unlock( device->lock );

    return 0;
}

int net_device_ioctl( int command, void* buffer, bool from_kernel ) {
    int error;

    switch ( command ) {
        case SIOCGIFCOUNT :
            error = net_device_get_count();
            break;

        case SIOCGIFCONF :
            error = get_interface_list( ( struct ifconf* )buffer );
            break;

        case SIOCGIFADDR :
        case SIOCGIFNETMASK :
        case SIOCGIFHWADDR :
        case SIOCGIFMTU :
        case SIOCGIFFLAGS :
        case SIOCGIFBRDADDR :
        case SIOCGIFSTAT :
            error = get_interface_parameter( command, ( struct ifreq* )buffer );
            break;

        case SIOCSIFADDR :
        case SIOCSIFNETMASK :
        case SIOCSIFBRDADDR :
        case SIOCSIFFLAGS :
            error = set_interface_parameter( command, ( struct ifreq* )buffer );
            break;

        case SIOCADDRT :
            error = route_add( ( struct rtentry* )buffer );;
            break;

        case SIOCDELRT :
            error = -ENOSYS;
            break;

        case SIOCGETRTAB :
            error = route_get_table( ( struct rttable* )buffer );
            break;

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

#endif /* ENABLE_NETWORK */
