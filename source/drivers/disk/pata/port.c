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

#include <types.h>
#include <console.h>
#include <thread.h>
#include <errno.h>

#include <arch/io.h>
#include <arch/pit.h> /* get_system_time() */

#include "pata.h"

int pata_port_wait( pata_port_t* port, uint8_t set, uint8_t clear, bool check_error, uint64_t timeout ) {
    uint64_t end_time;

    end_time = get_system_time() + timeout;

    inb( port->ctrl_base );
    thread_sleep( 1 );

    while ( get_system_time() < end_time ) {
        uint8_t status;

        status = inb( port->ctrl_base );

        if ( ( check_error ) && ( ( status & PATA_STATUS_ERROR ) != 0 ) ) {
            kprintf( ERROR, "PATA: pata_port_wait(): Error! (status=%x)\n", status );
            return -1;
        }

        if ( ( ( status & set ) == set ) && ( ( status & clear ) == 0 ) ) {
            return 0;
        }
    }

    return -ETIME;
}

void pata_port_select( pata_port_t* port ) {
    if ( port->is_slave ) {
        outb( PATA_SELECT_DEFAULT | PATA_SELECT_SLAVE, port->cmd_base + PATA_REG_DEVICE );
    } else {
        outb( PATA_SELECT_DEFAULT, port->cmd_base + PATA_REG_DEVICE );
    }

    inb( port->ctrl_base );
    thread_sleep( 1 );
}

bool pata_is_port_present( pata_port_t* port ) {
    uint8_t count;
    uint8_t lba_low;

    pata_port_select( port );

    outb( 0xAA, port->cmd_base + PATA_REG_COUNT );
    outb( 0x55, port->cmd_base + PATA_REG_LBA_LOW );

    inb( port->ctrl_base );
    thread_sleep( 1 );

    count = inb( port->cmd_base + PATA_REG_COUNT );
    lba_low = inb( port->cmd_base + PATA_REG_LBA_LOW );

    return ( ( count == 0xAA ) && ( lba_low == 0x55 ) );
}

static void extract_model_name( pata_identify_info_t* identify_info, char* name ) {
    int i;

    for ( i = 0; i < 40; i += 2 ) {
        *name++ = identify_info->model_id[ i + 1 ];
        *name++ = identify_info->model_id[ i ];
    }

    *name = 0;
}

int pata_port_identify( pata_port_t* port ) {
    int error;
    uint8_t command;

    /* Decide which PATA command to use */

    if ( port->is_atapi ) {
        command = PATA_CMD_IDENTIFY_PACKET_DEVICE;
    } else {
        command = PATA_CMD_IDENTIFY_DEVICE;
    }

    /* Send the command to the port */

    error = pata_port_send_command( port, command, !port->is_atapi, NULL, 0 );

    if ( error < 0 ) {
        return error;
    }

    /* Wait until the port clears the busy flag and data is available */

    error = pata_port_wait( port, PATA_STATUS_DRQ, PATA_STATUS_BUSY, false, port->is_atapi ? 20000000 : 500000 );

    if ( error < 0 ) {
        uint8_t err;

        err = inb( port->cmd_base + PATA_REG_ERROR );
        kprintf( ERROR, "PATA: Identify command timed out (error=%x)\n", err );

        return error;
    }

    /* Read the data from the port */

    pata_port_read_pio( port, &port->identify_info, sizeof( pata_identify_info_t ) / 2 );

    /* Wait until the port clears the DRQ bit */

    error = pata_port_wait( port, 0, PATA_STATUS_DRQ, true, 1000000 );

    if ( error < 0 ) {
        return error;
    }

    /* Finish the command */

    error = pata_port_finish_command( port, true, !port->is_atapi, 20000000 );

    if ( error < 0 ) {
        return error;
    }

    /* Parse some parts of the identify info data */

    extract_model_name( &port->identify_info, port->model_name );

    return 0;
}
