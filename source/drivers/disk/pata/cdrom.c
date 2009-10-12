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
#include <bitops.h>
#include <macros.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "pata.h"

typedef struct cdrom_capacity {
    uint32_t lba;
    uint32_t block_length;
} cdrom_capacity_t;

int pata_configure_atapi_port( pata_port_t* port ) {
    port->sector_size = 2048;

    return 0;
}

static int pata_cdrom_get_capacity( pata_port_t* port, uint64_t* capacity ) {
    int error;
    uint8_t packet[ 12 ];
    cdrom_capacity_t cdrom_capacity;

    /* Build the ATAPI packet */

    memset( packet, 0, sizeof( packet ) );
    packet[ 0 ] = ATAPI_CMD_READ_CAPACITY;

    /* Send the packet to the device */

    error = pata_port_atapi_do_packet( port, packet, true, &cdrom_capacity, sizeof( cdrom_capacity ) );

    if ( error < 0 ) {
        return error;
    }

    *capacity = ( 1 + bswap32( cdrom_capacity.lba ) ) * port->sector_size;

    return 0;
}

static int pata_cdrom_do_read( pata_port_t* port, void* buffer, uint64_t offset, uint32_t size ) {
    int error;
    uint64_t block;
    uint32_t block_count;
    uint8_t packet[ 12 ];

    block = offset / port->sector_size;
    block_count = size / port->sector_size;

    packet[ 0 ] = ATAPI_CMD_READ_10;
    packet[ 1 ] = 0;
    packet[ 2 ] = ( block >> 24 ) & 0xFF;
    packet[ 3 ] = ( block >> 16 ) & 0xFF;
    packet[ 4 ] = ( block >> 8 ) & 0xFF;
    packet[ 5 ] = block & 0xFF;
    packet[ 6 ] = ( block_count >> 16 ) & 0xFF;
    packet[ 7 ] = ( block_count >> 8 ) & 0xFF;
    packet[ 8 ] = block_count & 0xFF;
    packet[ 9 ] = 0;
    packet[ 10 ] = 0;
    packet[ 11 ] = 0;

    error = pata_port_atapi_do_packet( port, packet, true, buffer, size );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int pata_cdrom_open( void* node, uint32_t flags, void** cookie ) {
    int error;
    pata_port_t* port;

    port = ( pata_port_t* )node;

    mutex_lock( port->mutex, LOCK_IGNORE_SIGNAL );

    if ( port->open ) {
        mutex_unlock( port->mutex );

        return -EBUSY;
    }

    port->open = true;

    mutex_unlock( port->mutex );

    error = pata_cdrom_get_capacity( port, &port->capacity );

    if ( error < 0 ) {
        mutex_lock( port->mutex, LOCK_IGNORE_SIGNAL );

        port->open = false;

        mutex_unlock( port->mutex );

        return error;
    }

    return 0;
}

static int pata_cdrom_close( void* node, void* cookie ) {
    pata_port_t* port;

    port = ( pata_port_t* )node;

    mutex_lock( port->mutex, LOCK_IGNORE_SIGNAL );

    port->open = false;
    port->capacity = 0;

    mutex_unlock( port->mutex );

    return 0;
}

static int pata_cdrom_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;

    switch ( command ) {
        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static int pata_cdrom_read( void* node, void* cookie, void* buffer, off_t position, size_t size ) {
    int error;
    uint8_t* data;
    size_t saved_size;
    pata_port_t* port;

    port = ( pata_port_t* )node;

    if ( __unlikely( ( position % port->sector_size ) != 0 ) ) {
        return -EINVAL;
    }

    if ( __unlikely( ( size % port->sector_size ) != 0 ) ) {
        return -EINVAL;
    }

    if ( __unlikely( ( position + size ) > port->capacity ) ) {
        return -EINVAL;
    }

    if ( size == 0 ) {
        return 0;
    }

    data = ( uint8_t* )buffer;
    saved_size = size;

    mutex_lock( port->mutex, LOCK_IGNORE_SIGNAL );

    while ( size > 0 ) {
        size_t to_read = MIN( size, 32768 );

        error = pata_cdrom_do_read( port, data, position, to_read );

        if ( error < 0 ) {
            mutex_unlock( port->mutex );

            return error;
        }

        data += to_read;
        position += to_read;
        size -= to_read;
    }

    mutex_unlock( port->mutex );

    return saved_size;
}

static device_calls_t pata_cdrom_calls = {
    .open = pata_cdrom_open,
    .close = pata_cdrom_close,
    .ioctl = pata_cdrom_ioctl,
    .read = pata_cdrom_read,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int pata_create_atapi_device_node( pata_port_t* port ) {
    int error;
    char device[ 32 ];

    snprintf(
        device,
        sizeof( device ),
        "storage/od%c",
        '0' + 2 * port->channel + ( port->is_slave ? 1 : 0 )
    );

    kprintf( INFO, "PATA: Creating device node: /device/%s\n", device );

    error = create_device_node( device, &pata_cdrom_calls, ( void* )port );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
