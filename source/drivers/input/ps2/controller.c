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

#include <thread.h>
#include <console.h>
#include <errno.h>

#include <arch/spinlock.h>

#include "ps2.h"

int ps2_mux_enabled = 0;

static atomic_t ps2_ignore_int = ATOMIC_INIT(0);
static spinlock_t ps2_controller_lock = INIT_SPINLOCK( "PS/2 controller lock" );

void ps2_lock_controller( void ) {
    spinlock_disable( &ps2_controller_lock );
}

void ps2_unlock_controller( void ) {
    spinunlock_enable( &ps2_controller_lock );
}

int ps2_flush( void ) {
    int i;

    ps2_lock_controller();
    atomic_inc( &ps2_ignore_int );

    for ( i = 0; i < 64; i++ ) {
        if ( ( ps2_read_status() & PS2_STATUS_OBF ) == 0 ) {
            break;
        }

        ps2_read_data();

        udelay( 100 );
    }

    atomic_dec( &ps2_ignore_int );
    ps2_unlock_controller();

    return 0;
}

int ps2_wait_read( void ) {
    int i;

    for ( i = 0; i < PS2_CTRL_WAIT_TIMEOUT / 50; i++ ) {
        if ( ps2_read_status() & PS2_STATUS_OBF ) {
            return 0;
        }

        udelay( 50 );
    }

    return -EIO;
}

int ps2_wait_write( void ) {
    int i;

    for ( i = 0; i < PS2_CTRL_WAIT_TIMEOUT / 50; i++ ) {
        if ( ( ps2_read_status() & PS2_STATUS_IBF ) == 0 ) {
            return 0;
        }

        udelay( 50 );
    }

    return -EIO;
}

int ps2_interrupt( int irq, void* _data, registers_t* regs ) {
    uint8_t data;
    uint8_t status;
    ps2_device_t* device;

    status = ps2_read_status();

    if ( ( status & PS2_STATUS_OBF ) == 0 ) {
        return 0;
    }

    if ( atomic_get( &ps2_ignore_int ) > 0 ) {
        return 0;
    }

    data = ps2_read_data();

    if ( ( status & PS2_STATUS_AUX_DATA ) != 0 ) {
        int index;

        if ( ps2_mux_enabled ) {
            index = ( status >> 6 ) & 0x3;
        } else {
            index = 0;
        }

        device = &ps2_devices[ PS2_DEV_MOUSE + index ];
    } else {
        device = &ps2_devices[ PS2_DEV_KEYBOARD ];
    }

    return ps2_device_interrupt( device, status, data );
}

int ps2_command( uint8_t command, uint8_t* output, int out_count, uint8_t* input, int in_count ) {
    int i;
    int ret;

    ps2_lock_controller();
    atomic_inc( &ps2_ignore_int );

    ret = ps2_wait_write();

    if ( ret != 0 ) {
        goto out;
    }

    ps2_write_command( command );

    for ( i = 0; i < out_count; i++ ) {
        ret = ps2_wait_write();

        if ( ret != 0 ) {
            goto out;
        }

        ps2_write_data( output[ i ] );
    }

    for ( i = 0; i < in_count; i++ ) {
        ret = ps2_wait_read();

        if ( ret != 0 ) {
            goto out;
        }

        input[ i ] = ps2_read_data();
    }

 out:
    atomic_dec( &ps2_ignore_int );
    ps2_unlock_controller();

    return ret;
}

int ps2_selftest( void ) {
    int ret;
    uint8_t in;

    ret = ps2_command( PS2_CTRL_SELF_TEST, NULL, 0, &in, 1 );

    if ( ( ret != 0 ) ||
         ( in != 0x55 ) ) {
        kprintf( INFO, "ps2: controller self test failed, data: 0x%x.\n", in );
        return -1;
    }

    return 0;
}

int ps2_controller_init( void ) {
    int ret;
    uint8_t cmdbyte;

    ret = ps2_command( PS2_CTRL_READ_CMD, NULL, 0, &cmdbyte, 1 );

    if ( ret != 0 ) {
        cmdbyte = 0x47;
    }

    cmdbyte |= PS2_TRANSLATE | PS2_KBD_INT | PS2_AUX_INT;
    cmdbyte &= ~( PS2_KBD_DIS | PS2_AUX_DIS );

    ret = ps2_command( PS2_CTRL_WRITE_CMD, &cmdbyte, 1, NULL, 0 );

    if ( ret != 0 ) {
        kprintf( ERROR, "ps2: Failed to write control register.\n" );
    }

    return ret;
}
