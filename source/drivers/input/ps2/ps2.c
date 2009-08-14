/* PS/2 driver
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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
#include <errno.h>

#include <arch/io.h>
#include <arch/spinlock.h>

#include "ps2.h"

static spinlock_t ps2_spinlock = INIT_SPINLOCK( "PS/2 lock" );

static int ps2_wait_read( void ) {
    int i = PS2_WAIT_TIMEOUT;

    while( ( ~inb( PS2_PORT_STATUS ) & PS2_STATUS_OBF ) && ( i > 0 ) ) {
        //udelay( 50 );
        i--;
    }

    if ( i == 0 ) {
        kprintf( ERROR, "ps2_wait_read(): Timed out!\n" );
        return -ETIME;
    }

    return 0;
}

static int ps2_wait_write( void ) {
    int i = PS2_WAIT_TIMEOUT;

    while ( ( inb( PS2_PORT_STATUS ) & PS2_STATUS_IBF ) && ( i > 0 ) ) {
        //udelay( 50 );
        i--;
    }

    if ( i == 0 ) {
        kprintf( ERROR, "ps2_wait_write(): Timed out!\n" );
        return -ETIME;
    }

    return 0;
}

int ps2_flush_buffer( void ) {
    int i = 0;

    spinlock_disable( &ps2_spinlock );

    while ( ( inb( PS2_PORT_STATUS ) & PS2_STATUS_OBF ) && ( i < PS2_BUFSIZE ) ) {
        //udelay( 50 );
        inb( PS2_PORT_DATA );
        i++;
    }

    spinunlock_enable( &ps2_spinlock );

    return 0;
}

int ps2_command( uint8_t command ) {
    int error;

    spinlock_disable( &ps2_spinlock );

    error = ps2_wait_write();

    if ( error < 0 ) {
        goto out;
    }

    outb( command, PS2_PORT_COMMAND );

out:
    spinunlock_enable( &ps2_spinlock );

    return error;
}

int ps2_read_command( uint8_t command, uint8_t* data ) {
    int error;

    spinlock_disable( &ps2_spinlock );

    error = ps2_wait_write();

    if ( error < 0 ) {
        goto out;
    }

    outb( command, PS2_PORT_COMMAND );

    error = ps2_wait_read();

    if ( error < 0 ) {
        goto out;
    }

    *data = inb( PS2_PORT_DATA );

out:
    spinunlock_enable( &ps2_spinlock );

    return error;
}

int ps2_write_command( uint8_t command, uint8_t data ) {
    int error;

    spinlock_disable( &ps2_spinlock );

    error = ps2_wait_write();

    if ( error < 0 ) {
        goto out;
    }

    outb( command, PS2_PORT_COMMAND );

    error = ps2_wait_write();

    if ( error < 0 ) {
        goto out;
    }

    outb( data, PS2_PORT_DATA );

out:
    spinunlock_enable( &ps2_spinlock );

    return error;
}

int ps2_write_read_command( uint8_t command, uint8_t* data ) {
    int error;

    spinlock_disable( &ps2_spinlock );

    error = ps2_wait_write();

    if ( error < 0 ) {
        goto out;
    }

    outb( command, PS2_PORT_COMMAND );

    error = ps2_wait_write();

    if ( error < 0 ) {
        goto out;
    }

    outb( *data, PS2_PORT_DATA );

    error = ps2_wait_read();

    if ( error < 0 ) {
        goto out;
    }

    *data = inb( PS2_PORT_DATA );

out:
    spinunlock_enable( &ps2_spinlock );

    return error;
}

void ps2_lock( void ) {
    spinlock_disable( &ps2_spinlock );
}

void ps2_unlock( void ) {
    spinunlock_enable( &ps2_spinlock );
}

static int ps2_controller_test( void ) {
    int error;
    uint8_t data;

    error = ps2_read_command( PS2_CMD_TEST, &data );

    if ( error < 0 ) {
        return error;
    }

    if ( data != PS2_RET_CTL_TEST ) {
        kprintf( ERROR, "PS2: Contorller selftest failed!\n" );
        return -EINVAL;
    }

    return 0;
}

int init_module( void ) {
    int error;

    ps2_flush_buffer();

    error = ps2_controller_test();

    if ( error < 0 ) {
        return error;
    }

    ps2_init_keyboard();
    ps2_init_mouse();

    return 0;
}

int destroy_module( void ) {
    return 0;
}
