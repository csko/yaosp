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

#include <console.h>
#include <errno.h>
#include <ioctl.h>
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "ps2.h"

static int ps2_keyboard_probe( ps2_device_t* device ) {
    int error;
    uint8_t data;

    error = ps2_device_command( device, PS2_CMD_RESET, NULL, 0, &data, 1 );

    if ( ( error != 0 ) ||
         ( data != 0xAA ) ) {
        kprintf( ERROR, "ps2: Keyboard reset failed, data: 0x%x\n", data );
        return -ENODEV;
    }

    return 0;
}

static int ps2_keyboard_interrupt( ps2_device_t* device, uint8_t status, uint8_t data ) {
    ps2_keyboard_cookie_t* cookie;

    cookie = ( ps2_keyboard_cookie_t* )device->cookie;

    spinlock_disable( &cookie->buffer_lock );
    ps2_buffer_write( &cookie->buffer, &data, 1 );
    spinunlock_enable( &cookie->buffer_lock );

    semaphore_unlock( cookie->buffer_sync, 1 );

    return 0;
}

static int ps2_keyboard_open( void* node, uint32_t flags, void** _cookie ) {
    ps2_device_t* device;
    ps2_keyboard_cookie_t* cookie;

    device = &ps2_devices[ PS2_DEV_KEYBOARD ];

    if ( ps2_keyboard_probe( device ) != 0 ) {
        return -1;
    }

    cookie = ( ps2_keyboard_cookie_t* )kmalloc( sizeof( ps2_keyboard_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->buffer_sync = semaphore_create( "PS/2 keyboard buffer sync", 0 );

    ps2_buffer_init( &cookie->buffer );
    init_spinlock( &cookie->buffer_lock, "PS/2 buffer lock" );

    device->interrupt = ps2_keyboard_interrupt;
    device->cookie = cookie;

    atomic_or( &device->flags, PS2_FLAG_ENABLED );

    return 0;
}

static int ps2_keyboard_close( void* node, void* cookie ) {
    ps2_device_t* device;

    device = &ps2_devices[ PS2_DEV_KEYBOARD ];

    atomic_and( &device->flags, ~PS2_FLAG_ENABLED );

    device->interrupt = NULL;

    return 0;
}

static int ps2_keyboard_ioctl( void* node, void* _cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;
    ps2_device_t* device;
    ps2_keyboard_cookie_t* cookie;

    device = ( ps2_device_t* )node;
    cookie = ( ps2_keyboard_cookie_t* )device->cookie;

    switch ( command ) {
        case IOCTL_INPUT_KBD_GET_KEY_CODE :
            error = semaphore_timedlock( cookie->buffer_sync, 1, LOCK_IGNORE_SIGNAL, INFINITE_TIMEOUT );

            if ( error != 0 ) {
                break;
            }

            spinlock_disable( &cookie->buffer_lock );
            error = ps2_buffer_read( &cookie->buffer, args, 1 );
            spinunlock_enable( &cookie->buffer_lock );

            break;

        default :
            error = -EINVAL;
            break;
    }

    return error;
}

static device_calls_t ps2_keyboard_calls = {
    .open = ps2_keyboard_open,
    .close = ps2_keyboard_close,
    .ioctl = ps2_keyboard_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int ps2_keyboard_create( ps2_device_t* device ) {
    return create_device_node( "input/ps2/keyboard", &ps2_keyboard_calls, device );
}
