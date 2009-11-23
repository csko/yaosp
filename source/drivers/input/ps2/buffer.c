/* PS/2 driver
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

#include <macros.h>
#include <errno.h>

#include "ps2.h"

int ps2_buffer_add( ps2_buffer_t* buffer, uint8_t data ) {
    ASSERT( buffer->buffer_size <= PS2_BUFSIZE );

    if ( buffer->buffer_size == PS2_BUFSIZE ) {
        return -ENOMEM;
    }

    buffer->buffer[ buffer->write_pos ] = data;

    buffer->write_pos = ( buffer->write_pos + 1 ) % PS2_BUFSIZE;
    buffer->buffer_size++;

    semaphore_unlock( buffer->sync, 1 );

    return 0;
}

int ps2_buffer_size( ps2_buffer_t* buffer ) {
    return buffer->buffer_size;
}

int ps2_buffer_sync( ps2_buffer_t* buffer ) {
    semaphore_lock( buffer->sync, 1, LOCK_IGNORE_SIGNAL );

    return 0;
}

uint8_t ps2_buffer_get( ps2_buffer_t* buffer ) {
    uint8_t data;

    ASSERT( buffer->buffer_size > 0 );

    data = buffer->buffer[ buffer->read_pos ];
    buffer->read_pos = ( buffer->read_pos + 1 ) % PS2_BUFSIZE;
    buffer->buffer_size--;

    return data;
}

int ps2_buffer_init( ps2_buffer_t* buffer ) {
    buffer->read_pos = 0;
    buffer->write_pos = 0;
    buffer->buffer_size = 0;

    buffer->sync = semaphore_create( "PS/2 buffer semaphore", 0 );

    if ( buffer->sync < 0 ) {
        return buffer->sync;
    }

    return 0;
}
