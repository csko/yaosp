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

#include <types.h>

#include <arch/io.h>

#include "pata.h"

void pata_port_read_pio( pata_port_t* port, void* buffer, size_t word_count ) {
    if ( ( word_count & 2 ) != 0 ) {
        inws(
            ( uint16_t* )buffer,
            word_count,
            port->cmd_base + PATA_REG_DATA
        );
    } else {
        inls(
            ( uint32_t* )buffer,
            word_count / 2,
            port->cmd_base + PATA_REG_DATA
        );
    }
}

void pata_port_write_pio( pata_port_t* port, void* buffer, size_t word_count ) {
    if ( ( word_count & 2 ) != 0 ) {
        outws(
            ( uint16_t* )buffer,
            word_count,
            port->cmd_base + PATA_REG_DATA
        );
    } else {
        outls(
            ( uint32_t* )buffer,
            word_count / 2,
            port->cmd_base + PATA_REG_DATA
        );
    }
}
