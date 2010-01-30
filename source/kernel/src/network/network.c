/* Commong networking functions
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

#include <console.h>
#include <module.h>
#include <kernel.h>
#include <vfs/vfs.h>
#include <network/interface.h>
#include <network/arp.h>
#include <network/socket.h>
#include <network/tcp.h>
#include <network/udp.h>
#include <network/route.h>
#include <network/device.h>

__init int net_load_drivers( void ) {
    int f;
    dirent_t entry;

    f = open( "/yaosp/system/module/network", O_RDONLY );

    if ( f < 0 ) {
        kprintf( WARNING, "net: Failed to open /yaosp/system/module/network directory.\n" );
        return -1;
    }

    while ( getdents( f, &entry, sizeof( dirent_t ) ) > 0 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        load_module( entry.name );
    }

    close( f );

    return 0;
}

__init void init_network( void ) {
    bool network_enabled;

    net_device_init();

    init_routes();
    init_arp();
    init_socket();
    init_tcp();
    init_udp();

    /* Don't load network drivers and don't start network interfaces if
       networking is disabled by a kernel parameter. */

    if ( ( get_kernel_param_as_bool( "enable_network", &network_enabled ) == 0 ) &&
         ( !network_enabled ) ) {
        kprintf( INFO, "Networking is disabled by kernel parameter.\n" );
        return;
    }

    net_load_drivers();
    net_device_start();
}

#endif /* ENABLE_NETWORK */
