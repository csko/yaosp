/* Parallel AT Attachment driver
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

#include <console.h>
#include <errno.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "pata.h"

int pata_configure_ata_port( pata_port_t* port ) {
    port->use_lba = ( port->identify_info.lba_sectors != 0 );
    port->use_lba48 = ( ( port->identify_info.command_set_2 & 0x400 ) != 0 );

    if ( port->use_lba48 ) {
        port->capacity = ( uint64_t )port->identify_info.lba_capacity_48 * 512;
    } else if ( port->use_lba ) {
        port->capacity = ( uint64_t )port->identify_info.lba_sectors * 512;
    } else {
        /* TODO: CHS support */
        return -1;
    }

    port->sector_size = 512;

    return 0;
}

static int pata_disk_open( void* node, uint32_t flags, void** cookie ) {
    return -ENOSYS;
}

static int pata_disk_close( void* node, void* cookie ) {
    return -ENOSYS;
}

static int pata_disk_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    return -ENOSYS;
}

static int pata_disk_read( void* node, void* cookie, void* buffer, off_t position, size_t size ) {
    return -ENOSYS;
}

static int pata_disk_write( void* node, void* cookie, const void* buffer, off_t position, size_t size ) {
    return -ENOSYS;
}

static device_calls_t pata_disk_calls = {
    .open = pata_disk_open,
    .close = pata_disk_close,
    .ioctl = pata_disk_ioctl,
    .read = pata_disk_read,
    .write = pata_disk_write
};

int pata_create_ata_device_node( pata_port_t* port ) {
    int error;
    char device[ 32 ];

    snprintf(
        device,
        sizeof( device ),
        "disk/hd%c",
        'a' + 2 * port->channel + ( port->is_slave ? 1 : 0 )
    );

    kprintf( "PATA: Creating device node: /device/%s\n", device );

    error = create_device_node( device, &pata_disk_calls, ( void* )port );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
