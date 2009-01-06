/* PS/2 keyboard driver
 *
 * Copyright (c) 2009 Kornel Csernai
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

#include <irq.h>
#include <console.h>
#include <arch/io.h>
#include <vfs/devfs.h>

#include "ps2kbd.h"

static uint8_t ps2kbd_buffer[PS2KBD_BUFSIZE];
static int readpos = 0;
static int writepos = 0;
static int size = 0;

static semaphore_id sync;
static spinlock_t ps2kbd_lock;

static int ps2_keyboard_handler( int irq, void* data, registers_t* regs ) {

    int scancode = inb ( 0x60 );

    spinlock_disable( &ps2kbd_lock );

    if( size < PS2KBD_BUFSIZE ) { /* Buffer is not full, put the scancode in it */
        ps2kbd_buffer[writepos] = scancode;
        writepos = (writepos + 1) % PS2KBD_BUFSIZE;
        size++;
        UNLOCK( sync );
    }

    spinunlock_enable( &ps2kbd_lock );

    kprintf ( "PS2KBD: [0x%x]\n", scancode );

    return 0;
}

static int ps2kbd_read( void* node, void* cookie, void* buffer, off_t position, size_t size ) {

    int ret;

    spinlock_disable( &ps2kbd_lock );

    while ( size == 0 ) { /* Lock while there is nothing to be read */
        spinunlock_enable( &ps2kbd_lock );
        LOCK( sync );
        spinlock_disable( &ps2kbd_lock );
    }

    /* Buffer is not empty, there is something to be read */

    ret = ps2kbd_buffer[readpos];
    readpos = (readpos + 1) % PS2KBD_BUFSIZE;
    size--;

    spinunlock_enable( &ps2kbd_lock );

    return ret;
}

static device_calls_t ps2kbd_calls = {
    .open = NULL,
    .close = NULL,
    .ioctl = NULL,
    .read = ps2kbd_read,
    .write = NULL
};

int init_module( void ) {
    int error;

    error = request_irq(1, ps2_keyboard_handler, NULL);

    if ( error < 0 ) {
        kprintf( "PS2KBD: Failed to request IRQ for keyboard.\n" );
        return error;
    }

    sync = create_semaphore( "PS/2 keyboard sync", SEMAPHORE_COUNTING, 0, 0 );
    if ( sync < 0 ) {
        kprintf ( "PS2KBD: Failed to create a semaphore.\n" );
        return sync;
    }    

    error = create_device_node( "/device/input/ps2kbd", &ps2kbd_calls, NULL );
    if ( error < 0 ) {
        kprintf ( "PS2KBD: Failed to register device.\n" );
        return error;
    }

    kprintf ( "PS2KBD: Keyboard initialized.\n" );

    return 0;
}

int destroy_module( void ) {
    /* TODO: unregister the device */
    delete_semaphore( sync );
    return 0;
}
