/* PS/2 keyboard driver
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
#include <irq.h>
#include <errno.h>
#include <console.h>
#include <config.h>
#include <vfs/devfs.h>

#include <arch/io.h>

#include "ps2.h"

static ps2_buffer_t kbd_buffer;

#ifdef ENABLE_DEBUGGER
static uint8_t prev_scancode = 0;
#endif /* ENABLE_DEBUGGER */

static int ps2kbd_interrupt( int irq, void* _data, registers_t* regs ) {
    uint8_t data;

    ps2_lock();

    if ( ( inb( PS2_PORT_STATUS ) & PS2_STATUS_OBF ) != PS2_STATUS_OBF ) {
        ps2_unlock();

        return 0;
    }

    data = inb( PS2_PORT_DATA );

    ps2_buffer_add( &kbd_buffer, data );

    ps2_unlock();

#ifdef ENABLE_DEBUGGER
    if ( ( prev_scancode == 0xE0 ) &&
         ( data == 0x37 ) ) {
        __asm__ __volatile__( "int $0x1" );
    }

    prev_scancode = data;
#endif /* ENABLE_DEBUGGER */

    return 0;
}

static int ps2kbd_open( void* node, uint32_t flags, void** cookie ) {
    uint8_t control;

    ps2_flush_buffer();

    request_irq( 1, ps2kbd_interrupt, NULL );

    ps2_read_command( PS2_CMD_RCTR, &control );

    control &= ~PS2_CTR_KBDDIS;
    control |= PS2_CTR_KBDINT;

    ps2_write_command( PS2_CMD_WCTR, control );

    return 0;
}

static int ps2kbd_close( void* node, void* cookie ) {
    uint8_t control;

    ps2_read_command( PS2_CMD_RCTR, &control );

    control |= PS2_CTR_KBDDIS;
    control &= ~PS2_CTR_KBDINT;

    ps2_write_command( PS2_CMD_WCTR, control );

    /* TODO: release the IRQ */

    return 0;
}

static int ps2kbd_read( void* node, void* cookie, void* buffer, off_t position, size_t _size ) {
    int ret = 0;
    uint8_t* data;

    data = ( uint8_t* )buffer;

    ps2_lock();

    /* Lock while there is nothing to be read */

    while ( ps2_buffer_size( &kbd_buffer ) == 0 ) {
        ps2_unlock();
        ps2_buffer_sync( &kbd_buffer );
        ps2_lock();
    }

    /* Buffer is not empty, there is something to be read */

    while ( ( ps2_buffer_size( &kbd_buffer ) > 0 ) && ( _size > 0 ) ) {
        *data++ = ps2_buffer_get( &kbd_buffer );
        _size--;
        ret++;
    }

    ps2_unlock();

    return ret;
}

static int ps2kbd_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    return 0;
}

static device_calls_t ps2kbd_calls = {
    .open = ps2kbd_open,
    .close = ps2kbd_close,
    .ioctl = ps2kbd_ioctl,
    .read = ps2kbd_read,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int ps2_init_keyboard( void ) {
    int error;
    uint8_t control;

    /* Flush buffer */

    ps2_flush_buffer();

    /* Read control register */

    error = ps2_read_command( PS2_CMD_RCTR, &control );

    if ( error < 0 ) {
        kprintf( ERROR, "PS2: I/O error!\n" );
        return -EIO;
    }

    control |= PS2_CTR_KBDDIS;
    control &= ~PS2_CTR_KBDINT;

    /* Check if translated mode is enabled */

    if ( ( control & PS2_CTR_XLATE ) == 0 ) {
        kprintf( WARNING, "PS2: Keyboard is in non-translated mode.\n" );
        return -EINVAL;
    }

    /* Write control register */

    error = ps2_write_command( PS2_CMD_WCTR, control );

    if ( error < 0 ) {
        kprintf( ERROR, "PS2: I/O error!\n" );
        return -EIO;
    }

    error = ps2_buffer_init( &kbd_buffer );

    if ( error < 0 ) {
        kprintf( ERROR, "PS2: Failed to initialize keyboard buffer!\n" );
        return error;
    }

    error = create_device_node( "input/ps2kbd", &ps2kbd_calls, NULL );

    if ( error < 0 ) {
        return error;
    }

    kprintf( INFO, "PS2: Keyboard initialized!\n" );

    return 0;
}
