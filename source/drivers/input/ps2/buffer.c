/* PS/2 controller driver
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

#include <errno.h>

#include "ps2.h"

int ps2_buffer_init( ps2_buffer_t* buffer ) {
    buffer->read_pos = 0;
    buffer->write_pos = 0;
    buffer->free_size = PS2_BUF_SIZE;

    return 0;
}

int ps2_buffer_read( ps2_buffer_t* buffer, uint8_t* data, int size ) {
    int i;
    int data_size;

    data_size = PS2_BUF_SIZE - buffer->free_size;

    if ( data_size < size ) {
        return -EAGAIN;
    }

    for ( i = 0; i < size; i++, data++ ) {
        *data = buffer->data[ buffer->read_pos ];
        buffer->read_pos = ( buffer->read_pos + 1 ) % PS2_BUF_SIZE;
    }

    buffer->free_size += size;

    return 0;
}

int ps2_buffer_write( ps2_buffer_t* buffer, uint8_t* data, int size ) {
    int i;

    if ( buffer->free_size < size ) {
        return -ENOSPC;
    }

    for ( i = 0; i < size; i++, data++ ) {
        buffer->data[ buffer->write_pos ] = *data;
        buffer->write_pos = ( buffer->write_pos + 1 ) % PS2_BUF_SIZE;
    }

    buffer->free_size -= size;

    return 0;
}
