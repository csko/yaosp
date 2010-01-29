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
#include <input.h>
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "ps2.h"

int ps2_setup_active_multiplexing( int* enabled ) {
    int ret;
    uint8_t in;
    uint8_t out;

    out = 0xF0;
    ret = ps2_command( 0xD3, &out, 1, &in, 1 );

    if ( ret != 0 ) {
        goto fail;
    }

    // Step 1, if controller is good, in does match out.
    // This test failes with MS Virtual PC.

    if ( in != out ) {
        goto no_support;
    }

    out = 0x56;
    ret = ps2_command( 0xD3, &out, 1, &in, 1 );

    if ( ret != 0 ) {
        goto fail;
    }

    // Step 2, if controller is good, in does match out.

    if ( in != out ) {
        goto no_support;
    }

    out = 0xA4;
    ret = ps2_command( 0xD3, &out, 1, &in, 1 );

    if ( ret != 0 ) {
        goto fail;
    }

    // Step 3, if the controller doesn't support active multiplexing,
    // then in data does match out data (0xa4), else it's version number.

    if ( in == out ) {
        goto no_support;
    }

    // With some broken USB legacy emulation, it's 0xac, and with
    // MS Virtual PC, it's 0xa6. Since current active multiplexing
    // specification version is 1.1 (0x11), we validate the data.

    if ( in > 0x9F ) {
        kprintf( INFO, "ps2: Active multiplexing v%d.%d detected, but ignored!\n", ( in >> 4 ), in & 0xF );
        goto no_support;
    }

    kprintf( INFO, "ps2: Active multiplexing v%d.%d enabled.\n", ( in >> 4 ), in & 0xF );
    *enabled = 1;

    goto done;

 no_support:
    kprintf( INFO, "ps2: Active multiplexing not supported.\n" );
    *enabled = 0;

 done:
    // Some controllers get upset by the d3 command and will continue data loopback,
    // thus we need to send a harmless command (enable keyboard interface) next.

    ret = ps2_command( 0xAE, NULL, 0, NULL, 0 );

    if ( ret != 0 ) {
        kprintf( INFO, "ps2: active multiplexing 0xD3 workaround failed.\n" );
    }

    return 0;

 fail:
    *enabled = 0;

    // this should revert the controller into legacy mode,
    // just in case it has switched to multiplexed mode

    return ps2_selftest();
}

int ps2_reset_mouse( ps2_device_t* device ) {
    int ret;
    uint8_t data[ 2 ];

    ret = ps2_device_command( device, PS2_CMD_RESET, NULL, 0, data, 2 );

    if ( ( ret == 0 ) &&
         ( data[ 0 ] != 0xAA ) &&
         ( data[ 1 ] != 0x00 ) ) {
        kprintf( INFO, "ps2: Mouse reset failed, response was: 0x%x 0x%x.\n", data[ 0 ], data[ 1 ] );
        ret = -EINVAL;
    } else if ( ret != 0 ) {
        kprintf( INFO, "ps2: Mouse reset failed: %d.\n", ret );
    }

    return ret;
}

static inline int ps2_set_sample_rate( ps2_device_t* device, uint8_t rate ) {
    return ps2_device_command( device, PS2_CMD_SET_SAMPLE_RATE, &rate, 1, NULL, 0 );
}

int ps2_detect_mouse( ps2_device_t* device ) {
    int error;
    uint8_t deviceId = 0;

    error = ps2_reset_mouse( device );

    if ( error != 0 ) {
        return error;
    }

    /* Get device id */

    error = ps2_device_command( device, PS2_CMD_GET_DEVICE_ID, NULL, 0, &deviceId, 1 );

    if ( error != 0 ) {
        kprintf( INFO, "ps2: Get device id command failed.\n" );
        return -EIO;
    }

    /* Check for MS Intellimouse */

    if ( deviceId == 0 ) {
        uint8_t alternateDeviceId;

        error  = ps2_set_sample_rate( device, 200 );
        error |= ps2_set_sample_rate( device, 100 );
        error |= ps2_set_sample_rate( device, 80 );
        error |= ps2_device_command( device, PS2_CMD_GET_DEVICE_ID, NULL, 0, &alternateDeviceId, 1 );

        if ( error == 0 ) {
            deviceId = alternateDeviceId;
        }
    }

    switch ( deviceId ) {
        case PS2_DEV_ID_STANDARD :
            kprintf( INFO, "ps2: Standard PS/2 mouse found.\n" );
            device->packet_size = PS2_PACKET_STANDARD;
            break;

        case PS2_DEV_ID_INTELLIMOUSE :
            kprintf( INFO, "ps2: Extended PS/2 mouse found.\n" );
            device->packet_size = PS2_PACKET_INTELLIMOUSE;
            break;

        default :
            kprintf( ERROR, "ps2: Unknown device id found.\n" );
            return -EINVAL;
    }

    return 0;
}

static int ps2_mouse_interrupt( ps2_device_t* device, uint8_t status, uint8_t data ) {
    ps2_mouse_cookie_t* cookie;

    cookie = ( ps2_mouse_cookie_t* )device->cookie;

    if ( cookie->packet_index == 0 ) {
        if ( ( data & 0x08 ) == 0 ) {
            kprintf( WARNING, "ps2_mouse_interrupt(): Bad mouse data, resyncing ...\n" );
            return 0;
        }

        if ( data & 0xC0 ) {
            kprintf( WARNING, "ps2_mouse_interrupt(): X or Y overflow, resyncing ...\n" );
            return 0;
        }
    }

    cookie->packet[ cookie->packet_index++ ] = data;

    if ( cookie->packet_index == device->packet_size ) {
        ps2_lock_controller();

        if ( ( cookie->buffer_pos + device->packet_size ) < PS2_MOUSE_BUF_SIZE ) {
            memcpy( cookie->buffer + cookie->buffer_pos, cookie->packet, device->packet_size );
            cookie->buffer_pos += device->packet_size;

            semaphore_unlock( cookie->buffer_sync, 1 );
        }

        ps2_unlock_controller();

        cookie->packet_index = 0;
    }

    return 0;
}

static int ps2_mouse_open( void* node, uint32_t flags, void** _cookie ) {
    int error;
    ps2_device_t* device;
    ps2_mouse_cookie_t* cookie;

    device = ( ps2_device_t* )node;

    cookie = ( ps2_mouse_cookie_t* )kmalloc( sizeof( ps2_mouse_cookie_t ) );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    cookie->packet_index = 0;
    cookie->buffer_pos = 0;
    cookie->buffer_sync = semaphore_create( "PS/2 mouse buffer sync", 0 );

    device->interrupt = ps2_mouse_interrupt;
    device->cookie = cookie;

    error = ps2_device_command( device, PS2_CMD_ENABLE, NULL, 0, NULL, 0 );

    if ( error != 0 ) {
        kprintf( ERROR, "ps2: cannot enable mouse.\n" );
        device->interrupt = NULL;
        kfree( cookie );
        return error;
    }

    atomic_or( &device->flags, PS2_FLAG_ENABLED );

    return 0;
}

static int ps2_mouse_close( void* node, void* cookie ) {
    ps2_device_t* device;

    device = ( ps2_device_t* )node;

    atomic_and( &device->flags, ~PS2_FLAG_ENABLED );

    device->interrupt = NULL;

    return 0;
}

static int ps2_mouse_ioctl( void* node, void* _cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;
    ps2_device_t* device;
    ps2_mouse_cookie_t* cookie;

    device = ( ps2_device_t* )node;
    cookie = ( ps2_mouse_cookie_t* )device->cookie;

    switch ( command ) {
        case IOCTL_INPUT_MOUSE_GET_MOVEMENT : {
            uint8_t* packet;
            mouse_movement_t* data;

            semaphore_timedlock( cookie->buffer_sync, 1, LOCK_IGNORE_SIGNAL, INFINITE_TIMEOUT );

            packet = cookie->buffer;

            data = ( mouse_movement_t* )args;
            data->dx =    ( ( packet[ 0 ] & 0x10 ) ? 0xFFFFFF00 : 0 ) | packet[ 1 ];
            data->dy = -( ( ( packet[ 0 ] & 0x20 ) ? 0xFFFFFF00 : 0 ) | packet[ 2 ] );
            data->buttons = packet[ 0 ] & 0x7;

            if ( device->packet_size == 4 ) {
                data->scroll = packet[ 3 ] & 0x07;

                if ( packet[ 3 ] & 0x08 ) {
                    data->scroll |= ~0x07;
                }
            } else {
                data->scroll = 0;
            }

            ps2_lock_controller();

            if ( cookie->buffer_pos > device->packet_size ) {
                memmove( cookie->buffer, cookie->buffer + device->packet_size, cookie->buffer_pos - device->packet_size );
            }
            cookie->buffer_pos -= device->packet_size;

            ps2_unlock_controller();

            error = 0;
            break;
        }

        default :
            error = -EINVAL;
            break;
    }

    return error;
}

static device_calls_t ps2_mouse_calls = {
    .open = ps2_mouse_open,
    .close = ps2_mouse_close,
    .ioctl = ps2_mouse_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int ps2_mouse_create( ps2_device_t* device ) {
    return create_device_node( "input/ps2/mouse", &ps2_mouse_calls, device );
}
