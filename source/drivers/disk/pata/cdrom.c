/* Parallel AT Attachment driver
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#include <console.h>
#include <errno.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "pata.h"

int pata_configure_atapi_port( pata_port_t* port ) {
    return 0;
}

static int pata_cdrom_open( void* node, uint32_t flags, void** cookie ) {
    return -ENOSYS;
}

static int pata_cdrom_close( void* node, void* cookie ) {
    return -ENOSYS;
}

static int pata_cdrom_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    return -ENOSYS;
}

static int pata_cdrom_read( void* node, void* cookie, void* buffer, off_t position, size_t size ) {
    return -ENOSYS;
}

static device_calls_t pata_cdrom_calls = {
    .open = pata_cdrom_open,
    .close = pata_cdrom_close,
    .ioctl = pata_cdrom_ioctl,
    .read = pata_cdrom_read,
    .write = NULL
};

int pata_create_atapi_device_node( pata_port_t* port ) {
    int error;
    char device[ 32 ];

    snprintf(
        device,
        sizeof( device ),
        "disk/hd%c",
        'a' + 2 * port->channel + ( port->is_slave ? 1 : 0 )
    );

    kprintf( "PATA: Creating device node: /device/%s\n", device );

    error = create_device_node( device, &pata_cdrom_calls, ( void* )port );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
