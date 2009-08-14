/* Parallel AT Attachment driver
 *
 * Copyright (c) 2008 Zoltan Kovacs
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
#include <thread.h>
#include <macros.h>

#include <arch/io.h>

#include "pata.h"

int pata_port_send_command( pata_port_t* port, uint8_t cmd, bool check_drdy, uint8_t* regs, uint8_t reg_mask ) {
    int i;
    int error;
    uint8_t status;

    /* Select the port */

    pata_port_select( port );

    /* Wait for port to become idle */

    error = pata_port_wait( port, 0, PATA_STATUS_BUSY | PATA_STATUS_DRQ, 0, 100000 );

    if ( error < 0 ) {
        kprintf( ERROR, "PATA: Timed out while waiting for port to become idle\n" );
        return error;
    }

    /* Check data ready bit */

    if ( check_drdy ) {
        status = inb( port->ctrl_base );

        if ( ( status & PATA_STATUS_DRDY ) == 0 ) {
            return -1;
        }
    }

    /* Write command parameters */

    for ( i = 1; i < 7; i++ ) {
        if ( ( reg_mask & ( 1 << i ) ) != 0 ) {
            outb( regs[ i ], port->cmd_base + i );
        }
    }

    /* Send the command */

    outb( cmd, port->cmd_base + PATA_REG_COMMAND );

    /* Check error */

    status = inb( port->ctrl_base );

    if ( status & PATA_STATUS_ERROR ) {
        kprintf( ERROR, "PATA: Error after sending command!\n" );
        return -1;
    }

    return 0;
}

int pata_port_finish_command( pata_port_t* port, bool busy_wait, bool check_drdy, uint64_t timeout ) {
    uint8_t status;

    /* Wait for busy flag to clear if requested */

    if ( busy_wait ) {
        int error;

        error = pata_port_wait( port, 0, PATA_STATUS_BUSY, false, timeout );

        if ( error < 0 ) {
            return error;
        }
    }

    status = inb( port->cmd_base + PATA_REG_STATUS );

    if ( status & PATA_STATUS_BUSY ) {
        kprintf( ERROR, "PATA: Port is still busy when finishing the command!\n" );
        return -1;
    }

    if ( check_drdy ) {
        if ( ( status & PATA_STATUS_DRDY ) == 0 ) {
            return -1;
        }
    }

    if ( status & PATA_STATUS_ERROR ) {
        return -1;
    }

    return 0;
}

int pata_port_atapi_do_packet( pata_port_t* port, uint8_t* packet, bool do_read, void* buffer, size_t size ) {
    int error;
    char* data;
    uint16_t count;
    uint8_t cmd_data[ 7 ];

    if ( __unlikely( !port->is_atapi ) ) {
        kprintf( ERROR, "PATA: ATAPI operation not allowed on non-ATAPI drive!\n" );
        return -EINVAL;
    }

    cmd_data[ PATA_REG_FEATURE ] = 0;
    cmd_data[ PATA_REG_LBA_MID ] = size & 0xFF;
    cmd_data[ PATA_REG_LBA_HIGH ] = ( size >> 8 ) & 0xFF;

    /* Send the command */

    error = pata_port_send_command(
        port,
        PATA_CMD_PACKET,
        false,
        cmd_data,
        ( 1 << PATA_REG_FEATURE ) | ( 1 << PATA_REG_LBA_MID ) | ( 1 << PATA_REG_LBA_HIGH )
    );

    if ( error < 0 ) {
        return error;
    }

    /* Wait for device to get ready for packet transmission */

    error = pata_port_wait( port, PATA_STATUS_DRQ, PATA_STATUS_BUSY, true, 100000 );

    if ( error < 0 ) {
        return error;
    }

    /* TODO: check if the device really asks for command packet */

    thread_sleep( 10 );

    /* Write the packet to the device */

    pata_port_write_pio( port, packet, 6 );

    /* Wait for BUSY flag to clear */

    error = pata_port_wait( port, 0, PATA_STATUS_BUSY, true, 10000000 );

    if ( error < 0 ) {
        return error;
    }

    if ( do_read ) {
        data = ( char* )buffer;

        while ( size > 0 ) {
            /* Wait for DRQ */

            error = pata_port_wait( port, PATA_STATUS_DRQ, 0, true, 1000000 );

            if ( error < 0 ) {
                return error;
            }

            /* Get how much data the device wants to transmit */

            count =
                ( inb( port->cmd_base + PATA_REG_LBA_HIGH ) << 8 ) |
                inb( port->cmd_base + PATA_REG_LBA_MID );

            /* Receive the data from the device */

            pata_port_read_pio( port, data, count / 2 );

            size -= count;
            data += count;
        }
    } else {
        kprintf( WARNING, "PATA: ATAPI write not supported!\n" );
        return -EINVAL;
    }

    /* Wait for DRQ to clear */

    error = pata_port_wait( port, 0, PATA_STATUS_DRQ, true, 1000000 );

    if ( error < 0 ) {
        return error;
    }

    /* Finish the command */

    error = pata_port_finish_command( port, true, false, 20000000 );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
