/* PS/2 keyboard driver
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
#include <irq.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include <arch/io.h>
#include <arch/bios.h>

#include "ps2.h"

static ps2_buffer_t mouse_buffer;

static int ps2mouse_interrupt( int irq, void* _data, registers_t* regs ) {
    uint8_t data;

    ps2_lock();

    if ( ( inb( PS2_PORT_STATUS ) & PS2_STATUS_OBF ) != PS2_STATUS_OBF ) {
        goto out;
    }

    data = inb( PS2_PORT_DATA );

    ps2_buffer_add( &mouse_buffer, data );

out:
    ps2_unlock();

    return 0;
}

static int ps2mouse_open( void* node, uint32_t flags, void** cookie ) {
    uint8_t control;

    ps2_flush_buffer();

    request_irq( 12, ps2mouse_interrupt, NULL );

    ps2_command( PS2_CMD_AUX_ENABLE );

    ps2_read_command( PS2_CMD_RCTR, &control );

    control &= ~PS2_CTR_AUXDIS;
    control |= PS2_CTR_AUXINT;

    ps2_write_command( PS2_CMD_WCTR, control );

    return 0;
}

static int ps2mouse_close( void* node, void* cookie ) {
    uint8_t control;

    ps2_read_command( PS2_CMD_RCTR, &control );

    control |= PS2_CTR_AUXDIS;
    control &= ~PS2_CTR_AUXINT;

    ps2_write_command( PS2_CMD_WCTR, control );

    /* TODO: release the IRQ */

    return 0;
}

static int ps2mouse_read( void* node, void* cookie, void* buffer, off_t position, size_t _size ) {
    int ret = 0;
    uint8_t* data;

    data = ( uint8_t* )buffer;

    ps2_lock();

    /* Lock while there is nothing to be read */

    while ( ps2_buffer_size( &mouse_buffer ) == 0 ) {
        ps2_unlock();
        ps2_buffer_sync( &mouse_buffer );
        ps2_lock();
    }

    /* Buffer is not empty, there is something to be read */

    while ( ( ps2_buffer_size( &mouse_buffer ) > 0 ) && ( _size > 0 ) ) {
        *data++ = ps2_buffer_get( &mouse_buffer );
        _size--;
        ret++;
    }

    ps2_unlock();

    return ret;
}

static int ps2mouse_write( void* node, void* cookie, const void* buffer, off_t position, size_t size ) {
    size_t i;
    uint8_t* data;

    data = ( uint8_t* )buffer;

    for ( i = 0; i < size; i++ ) {
        ps2_write_command( PS2_CMD_AUX_SEND, *data++ );
    }

    return 0;
}

static device_calls_t ps2mouse_calls = {
    .open = ps2mouse_open,
    .close = ps2mouse_close,
    .ioctl = NULL,
    .read = ps2mouse_read,
    .write = ps2mouse_write,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int ps2_init_mouse( void ) {
    int error;
    uint8_t data;
    bios_regs_t regs;

    /* Check if the pointing device is present */

    memset( &regs, 0, sizeof( bios_regs_t ) );

    call_bios_interrupt( 0x11, &regs );

    if ( ( regs.ax & 0x04 ) == 0 ) {
        kprintf( INFO, "PS2: Pointing device not present!\n" );
        return -ENOENT;
    }

    /* Flush the buffer */

    ps2_flush_buffer();

    /* Test loop command */

    data = 0x5A;

    error = ps2_write_read_command( PS2_CMD_AUX_LOOP, &data );

    if ( ( error < 0 ) || ( data != 0x5A ) ) {
        kprintf( ERROR, "PS2: AUX loop test failed (error=%d, data=%x)!\n", error, ( uint32_t )data );

        if ( ps2_read_command( PS2_CMD_AUX_TEST, &data ) < 0 ) {
            kprintf( INFO, "PS2: Aux port not present!\n" );
            return -ENOENT;
        }

        kprintf( DEBUG, "PS2: Test command returned %x\n", ( uint32_t )data );

        if ( ( data ) && ( ( data != 0xFA ) && ( data != 0xFF ) ) ) {
            kprintf( ERROR, "PS2: Invalid return code!\n" );
            return -ENOENT;
        }
    }

    /* Disable and then enable the auxport */

    if ( ps2_command( PS2_CMD_AUX_DISABLE ) < 0 ) {
        return -ENOENT;
    }

    if ( ps2_command( PS2_CMD_AUX_ENABLE ) < 0 ) {
        return -ENOENT;
    }

    if ( ( ps2_read_command( PS2_CMD_RCTR, &data ) < 0 ) ||
         ( ( data & PS2_CTR_AUXDIS ) != 0 ) ) {
        return -EIO;
    }

    /* Disable aux port */

    data |= PS2_CTR_AUXDIS;
    data &= ~PS2_CTR_AUXINT;

    /* Write control register */

    error = ps2_write_command( PS2_CMD_WCTR, data );

    if ( error < 0 ) {
        kprintf( ERROR, "PS2: I/O error\n" );
        return -EIO;
    }

    kprintf( INFO, "PS2: AUX port detected\n" );

    error = ps2_buffer_init( &mouse_buffer );

    if ( error < 0 ) {
        kprintf( ERROR, "PS2: Failed to initialize mouse buffer!\n" );
        return error;
    }

    error = create_device_node( "input/ps2mouse", &ps2mouse_calls, NULL );

    if ( error < 0 ) {
        return error;
    }

    kprintf( INFO, "PS2: Mouse initialized!\n" );

    return 0;
}
