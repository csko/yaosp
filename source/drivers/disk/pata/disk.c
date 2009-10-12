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
#include <macros.h>
#include <ioctl.h>
#include <devices.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include <arch/io.h>

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

static int pata_disk_do_transfer( pata_port_t* port, void* buffer, uint64_t offset, uint32_t size, bool do_read ) {
    uint8_t* data;
    bool need_lba48;
    uint64_t sector;
    uint32_t sector_count;

    int error;
    uint8_t command;
    uint8_t cmd_data[ 7 ];
    uint8_t cmd_mask;

    sector = offset / port->sector_size;
    sector_count = size / port->sector_size;

    need_lba48 = ( ( sector + sector_count ) > 0xFFFFFFF ) || ( sector_count >= 0x100 );

    if ( ( need_lba48 ) && ( !port->use_lba48 ) ) {
        kprintf( WARNING, "PATA: Transfer requires LBA48, but it's not supported by the controller!\n" );
        return -EIO;
    }

    if ( need_lba48 ) {
        kprintf( WARNING, "PATA: LBA48 not yet supported!\n" );
        return -EIO;
    } else if ( port->use_lba ) {
        command = do_read ? PATA_CMD_READ_SECTORS : PATA_CMD_WRITE_SECTORS;

        cmd_data[ PATA_REG_FEATURE ] = 0;
        cmd_data[ PATA_REG_LBA_LOW ] = sector & 0xFF;
        cmd_data[ PATA_REG_LBA_MID ] = ( sector >> 8 ) & 0xFF;
        cmd_data[ PATA_REG_LBA_HIGH ] = ( sector >> 16 ) & 0xFF;
        cmd_data[ PATA_REG_DEVICE ] = PATA_DEVICE_LBA | ( ( sector >> 24 ) & 0x0F );
        cmd_data[ PATA_REG_COUNT ] = sector_count;

        cmd_mask = ( 1 << PATA_REG_FEATURE ) |
                   ( 1 << PATA_REG_LBA_LOW ) |
                   ( 1 << PATA_REG_LBA_MID ) |
                   ( 1 << PATA_REG_LBA_HIGH ) |
                   ( 1 << PATA_REG_DEVICE ) |
                   ( 1 << PATA_REG_COUNT );
    } else {
        return -EIO;
    }

    error = pata_port_send_command( port, command, true, cmd_data, cmd_mask );

    if ( error < 0 ) {
        return error;
    }

    data = ( uint8_t* )buffer;

    while ( sector_count > 0 ) {
        error = pata_port_wait( port, PATA_STATUS_DRQ, PATA_STATUS_BUSY, false, 1000000 );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }

        if ( do_read ) {
            pata_port_read_pio( port, data, port->sector_size / 2 );
        } else {
            pata_port_write_pio( port, data, port->sector_size / 2 );
        }

        data += port->sector_size;
        sector_count--;

        /* Wait 1 PIO cycle */

        if ( sector_count > 0 ) {
            inb( port->ctrl_base );
        }
    }

    error = pata_port_wait( port, 0, PATA_STATUS_DRQ, true, 1000000 );

    if ( error < 0 ) {
        return error;
    }

    error = pata_port_finish_command( port, true, true, 20000000 );

    if ( error < 0 ) {
      return error;
    }

    return 0;
}

static int pata_disk_open( void* node, uint32_t flags, void** cookie ) {
    return 0;
}

static int pata_disk_close( void* node, void* cookie ) {
    return 0;
}

static int pata_disk_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;
    pata_port_t* port;

    port = ( pata_port_t* )node;

    switch ( command ) {
        case IOCTL_DISK_GET_GEOMETRY : {
            device_geometry_t* geometry;

            geometry = ( device_geometry_t* )args;

            geometry->bytes_per_sector = port->sector_size;
            geometry->sector_count = port->capacity / port->sector_size;

            error = 0;

            break;
        }

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static int pata_disk_read( void* node, void* cookie, void* buffer, off_t position, size_t size ) {
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

        error = pata_disk_do_transfer( port, data, position, to_read, true );

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

static int pata_disk_write( void* node, void* cookie, const void* buffer, off_t position, size_t size ) {
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
        size_t to_write = MIN( size, 32768 );

        error = pata_disk_do_transfer( port, data, position, to_write, false );

        if ( error < 0 ) {
            mutex_unlock( port->mutex );

            return error;
        }

        data += to_write;
        position += to_write;
        size -= to_write;
    }

    mutex_unlock( port->mutex );

    return saved_size;
}

static device_calls_t pata_disk_calls = {
    .open = pata_disk_open,
    .close = pata_disk_close,
    .ioctl = pata_disk_ioctl,
    .read = pata_disk_read,
    .write = pata_disk_write,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int pata_create_ata_device_node( pata_port_t* port ) {
    int error;
    char device[ 32 ];

    snprintf(
        device,
        sizeof( device ),
        "storage/hd%c",
        '0' + 2 * port->channel + ( port->is_slave ? 1 : 0 )
    );

    kprintf( INFO, "PATA: Creating device node: /device/%s\n", device );

    error = create_device_node( device, &pata_disk_calls, ( void* )port );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
