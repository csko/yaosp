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
#include <semaphore.h>
#include <errno.h>
#include <console.h>
#include <vfs/devfs.h>

#include <arch/io.h>

#include "ps2.h"

static int kbd_read_pos = 0;
static int kbd_write_pos = 0;
static int kbd_buffer_size = 0;
static semaphore_id kbd_sync = -1;
static uint8_t kbd_buffer[ PS2_KBD_BUFSIZE ];

static int ps2kbd_interrupt( int irq, void* _data, registers_t* regs ) {
    uint8_t data;

    ps2_lock();

    if ( ( inb( PS2_PORT_STATUS ) & PS2_STATUS_OBF ) != PS2_STATUS_OBF ) {
        goto out;
    }

    data = inb( PS2_PORT_DATA );

    if ( kbd_buffer_size < PS2_KBD_BUFSIZE ) {
        kbd_buffer[ kbd_write_pos ] = data;

        kbd_write_pos = ( kbd_write_pos + 1 ) % PS2_KBD_BUFSIZE;
        kbd_buffer_size++;

        UNLOCK( kbd_sync );
    }

out:
    ps2_unlock();

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

    kbd_read_pos = 0;
    kbd_write_pos = 0;
    kbd_buffer_size = 0;

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

    while ( kbd_buffer_size == 0 ) {
        ps2_unlock();
        LOCK( kbd_sync );
        ps2_lock();
    }

    /* Buffer is not empty, there is something to be read */

    while ( ( kbd_buffer_size > 0 ) && ( _size > 0 ) ) {
        *data++ = kbd_buffer[ kbd_read_pos ];
        kbd_read_pos = ( kbd_read_pos + 1 ) % PS2_KBD_BUFSIZE;
        kbd_buffer_size--;
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
    .write = NULL
};

int ps2_init_keyboard( void ) {
    int error;
    uint8_t control;

    /* Flush buffer */

    ps2_flush_buffer();
    
    /* Read control register */

    error = ps2_read_command( PS2_CMD_RCTR, &control );
    
    if ( error < 0 ) {
        kprintf( "PS2: I/O error!\n" );
        return -EIO;
    }

    control |= PS2_CTR_KBDDIS;
    control &= ~PS2_CTR_KBDINT;
    
    /* Check if translated mode is enabled */

    if ( ( control & PS2_CTR_XLATE ) == 0 ) {
        kprintf( "PS2: Keyboard is in non-translated mode.\n" );
        return -EINVAL;
    }
    
    /* Write control register */

    error = ps2_write_command( PS2_CMD_WCTR, control );

    if ( error < 0 ) {
        kprintf( "PS2: I/O error!\n" );
        return -EIO;
    }

    kbd_sync = create_semaphore( "PS2 keyboard sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( kbd_sync < 0 ) {
        kprintf( "PS2: Failed to create keyboard sync semaphore!\n" );
        return kbd_sync;
    }

    create_device_node( "input/ps2kbd", &ps2kbd_calls, NULL );

    kprintf( "PS2: Keyboard initialized!\n" );

    return 0;
}
