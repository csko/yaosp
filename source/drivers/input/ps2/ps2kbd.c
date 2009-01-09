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
#include <errno.h>
#include <ioctl.h>
#include <vfs/devfs.h>

#include <arch/io.h>

#include "ps2kbd.h"

static uint8_t ps2kbd_buffer[PS2KBD_BUFSIZE];
static int readpos = 0;
static int writepos = 0;
static int size = 0;
static uint8_t kbd_status = 0; /* LEDs */

static semaphore_id sync;
static spinlock_t ps2kbd_lock = INIT_SPINLOCK;

static int ps2_keyboard_handler( int irq, void* data, registers_t* regs ) {
    uint8_t scancode = inb( PS2KBD_PORT_KBD );

    spinlock_disable( &ps2kbd_lock );

    if( size < PS2KBD_BUFSIZE ) { /* Buffer is not full, put the scancode in it */
        ps2kbd_buffer[writepos] = scancode;
        writepos = (writepos + 1) % PS2KBD_BUFSIZE;
        size++;
        UNLOCK( sync );
    }

    spinunlock_enable( &ps2kbd_lock );

    return 0;
}

static void ps2_keyboard_flush( void ) {
    while ( inb( PS2KBD_PORT_CONT ) & 0x01 ) {
        inb( PS2KBD_PORT_KBD );
    }
}

static int ps2kbd_open( void* node, uint32_t flags, void** cookie ) {
    readpos = 0;
    writepos = 0;
    size = 0;

    return 0;
}

static int ps2kbd_close( void* node, void* cookie ) {
    return 0;
}

static int ps2kbd_read( void* node, void* cookie, void* buffer, off_t position, size_t _size ) {
    int ret = 0;
    uint8_t* data;

    data = ( uint8_t* )buffer;

    spinlock_disable( &ps2kbd_lock );

    while ( size == 0 ) { /* Lock while there is nothing to be read */
        spinunlock_enable( &ps2kbd_lock );
        LOCK( sync );
        spinlock_disable( &ps2kbd_lock );
    }

    /* Buffer is not empty, there is something to be read */

    while ( ( size > 0 ) && ( _size > 0 ) ) {
        *data++ = ps2kbd_buffer[readpos];
        readpos = (readpos + 1) % PS2KBD_BUFSIZE;
        size--;
        _size--;
        ret++;
    }

    spinunlock_enable( &ps2kbd_lock );

    return ret;
}


static void ps2kbd_ack( void ) {
    while( ! ( inb( PS2KBD_PORT_KBD ) == 0xFA ) ) ;
}

static void ps2kbd_wait( void ) {
    while( inb( PS2KBD_PORT_CONT ) & (0x02 | 0x01) ) ;
}

static int ps2kbd_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error = 0;

    switch ( command ) {
        case IOCTL_PS2KBD_TOGGLE_LEDS : {
            int to_toggle = ( int )args;

            /* Only the 3 least significant bits are used, the rest must be 0 */

            kbd_status ^= ( ( uint8_t )to_toggle & 0x07 );

            ps2kbd_wait();
            outb( PS2KBD_CMD_LED, PS2KBD_PORT_KBD ); /* LED update command */

            ps2kbd_ack();

            ps2kbd_wait();
            outb( kbd_status, PS2KBD_PORT_KBD ); /* New LED status */

            break;
        }

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static device_calls_t ps2kbd_calls = {
    .open = ps2kbd_open,
    .close = ps2kbd_close,
    .ioctl = ps2kbd_ioctl,
    .read = ps2kbd_read,
    .write = NULL
};

int init_module( void ) {
    int error;

    ps2_keyboard_flush();

    /* TODO: Detect if hardware is present, cable plugged in, etc */

    /* Test the controller */
    ps2kbd_wait();
    outb( PS2KBD_CMD_STEST, PS2KBD_PORT_CONT );
    error = inb( PS2KBD_PORT_KBD );

    if ( error != 0x55 ) {
        kprintf( "PS2KBD: Hardware error (%d => 0x%x).\n", PS2KBD_CMD_STEST, error );
        return -EHW;
    }

    /* Test the interface */
    ps2kbd_wait();
    outb( PS2KBD_CMD_KTEST, PS2KBD_PORT_CONT );
    error = inb( PS2KBD_PORT_KBD );

    if ( error != 0x00 ) {
        kprintf( "PS2KBD: Hardware error (%d => 0x%x).\n", PS2KBD_CMD_KTEST, error );
        return -EHW;
    }

    /* Enable keyboard */
    ps2kbd_wait();
    outb( PS2KBD_CMD_ENABLE, PS2KBD_PORT_CONT );

    error = request_irq( 1, ps2_keyboard_handler, NULL );

    if ( error < 0 ) {
        kprintf( "PS2KBD: Failed to request IRQ for keyboard.\n" );
        return error;
    }

    sync = create_semaphore( "PS/2 keyboard sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( sync < 0 ) {
        kprintf( "PS2KBD: Failed to create a semaphore.\n" );
        return sync;
    }    

    error = create_device_node( "input/ps2kbd", &ps2kbd_calls, NULL );

    if ( error < 0 ) {
        kprintf( "PS2KBD: Failed to register device.\n" );
        return error;
    }

    kprintf( "PS2KBD: Keyboard initialized.\n" );

    return 0;
}

int destroy_module( void ) {
    /* TODO: unregister the device */
    delete_semaphore( sync );
    return 0;
}
