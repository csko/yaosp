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

#include <mm/kmalloc.h>
#include <network/device.h>

net_device_t* net_device_create( size_t priv_size ) {
    net_device_t* device;

    device = ( net_device_t* )kmalloc( sizeof( net_device_t ) + priv_size );
    device->private = ( void* )( device + 1 );

    return device;
}

int net_device_free( net_device_t* device ) {
    kfree( device );

    return 0;
}

int net_device_register( net_device_t* device ) {
    return 0;
}

void* net_device_get_private( net_device_t* device ) {
    return device->private;
}
