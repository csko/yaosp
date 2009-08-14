/* PS/2 mouse driver
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

#include <thread.h>
#include <console.h>
#include <kernel.h>
#include <ioctl.h>
#include <errno.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>

#include "../input.h"

#define PS2_AUX_SET_RES     0xE8
#define PS2_AUX_SET_SCALE11 0xE6
#define PS2_AUX_SET_SCALE21 0xE7
#define PS2_AUX_GET_SCALE   0xE9
#define PS2_AUX_SET_STREAM  0xEA
#define PS2_AUX_SET_SAMPLE  0xF3
#define PS2_AUX_ENABLE_DEV  0xF4
#define PS2_AUX_DISABLE_DEV 0xF5
#define PS2_AUX_RESET       0xFF
#define PS2_AUX_ACK         0xFA
#define PS2_AUX_SEND_ID     0xF2

#define PS2_AUX_ID_ERROR -1
#define PS2_AUX_ID_PS2   0
#define PS2_AUX_ID_IMPS2 3

enum {
    PS2_BUTTON_LEFT = ( 1 << 0 ),
    PS2_BUTTON_RIGHT = ( 1 << 1 ),
    PS2_BUTTON_MIDDLE = ( 1 << 2 )
};

static int mouse_device = -1;
static thread_id mouse_thread = -1;

static int mouse_packet_index = 0;
static uint8_t mouse_buffer[ 3 ];

static int mouse_buttons = 0;

static uint8_t basic_init[] = {
    PS2_AUX_ENABLE_DEV, PS2_AUX_SET_SAMPLE, 100
};

static uint8_t imps2_init[] = {
    PS2_AUX_SET_SAMPLE, 200, PS2_AUX_SET_SAMPLE, 100, PS2_AUX_SET_SAMPLE, 80
};

static uint8_t ps2_init[] = {
    PS2_AUX_SET_SCALE11, PS2_AUX_ENABLE_DEV, PS2_AUX_SET_SAMPLE, 100, PS2_AUX_SET_RES, 3
};

static int ps2_mouse_write( uint8_t* data, size_t size ) {
    size_t i;
    int error;
    uint8_t ack;

    error = 0;

    for ( i = 0; i < size; i++, data++ ) {
        pwrite( mouse_device, data, 1, 0 );

        if ( ( pread( mouse_device, &ack, 1, 0 ) != 1 ) ||
             ( ack != PS2_AUX_ACK ) ) {
            error++;
        }
    }

    return error;
}

static int ps2_mouse_read_id( void ) {
    int error;
    uint8_t data;

    data = PS2_AUX_SEND_ID;

    error = pwrite( mouse_device, &data, 1, 0 );

    if ( error < 0 ) {
        return PS2_AUX_ID_ERROR;
    }

    if ( ( pread( mouse_device, &data, 1, 0 ) != 1 ) ||
         ( data != PS2_AUX_ACK ) ) {
        return PS2_AUX_ID_ERROR;
    }

    if ( pread( mouse_device, &data, 1, 0 ) != 1 ) {
        return PS2_AUX_ID_ERROR;
    }

    return data;
}

static int ps2_mouse_init( void ) {
    int id;
    int error;

    mouse_device = open( "/device/input/ps2mouse", O_RDONLY );

    if ( mouse_device < 0 ) {
        return mouse_device;
    }

    ps2_mouse_write( basic_init, sizeof( basic_init ) );

    error = ps2_mouse_write( basic_init, sizeof( basic_init ) );

    if ( error < 0 ) {
        return error;
    }

    error = ps2_mouse_write( imps2_init, sizeof( imps2_init ) );

    if ( error < 0 ) {
        return error;
    }

    id = ps2_mouse_read_id();

    if ( id == PS2_AUX_ID_ERROR ) {
        kprintf( ERROR, "PS/2 mouse: Invalid mouse ID: %x\n", id );
        return -EINVAL;
    }

    error = ps2_mouse_write( ps2_init, sizeof( ps2_init ) );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static void ps2_mouse_handle_input( void ) {
    int x;
    int y;
    int buttons;
    int act_buttons;

    x = mouse_buffer[ 1 ];
    y = mouse_buffer[ 2 ];

    if ( mouse_buffer[ 0 ] & 0x10 ) {
        x |= 0xFFFFFF00;
    }

    if ( mouse_buffer[ 0 ] & 0x20 ) {
        y |= 0xFFFFFF00;
    }

    if ( ( x != 0 ) || ( y != 0 ) ) {
        input_event_t event;

        event.event = E_MOUSE_MOVED;
        event.param1 = x;
        event.param2 = -y;

        insert_input_event( &event );
    }

    buttons = mouse_buffer[ 0 ] & 0x7;

    act_buttons = buttons ^ mouse_buttons;
    mouse_buttons = buttons;

    /* Check if any button state changed */

    if ( act_buttons != 0 ) {
        if ( act_buttons & PS2_BUTTON_LEFT ) {
            input_event_t event;

            if ( buttons & PS2_BUTTON_LEFT ) {
                event.event = E_MOUSE_PRESSED;
            } else {
                event.event = E_MOUSE_RELEASED;
            }

            event.param1 = MOUSE_BTN_LEFT;
            event.param2 = 0;

            insert_input_event( &event );
        }
    }
}

static int ps2_mouse_thread( void* arg ) {
    uint8_t data;

    while ( 1 ) {
        if ( pread( mouse_device, &data, 1, 0 ) != 1 ) {
            kprintf( ERROR, "PS2mouse: Failed to read data from the device!\n" );
            break;
        }

        if ( ( mouse_packet_index >= 3 ) &&
             ( data & 0x08 ) ) {
            mouse_packet_index = 0;
        }

        switch ( mouse_packet_index ) {
            case 0 :
            case 1 :
                mouse_buffer[ mouse_packet_index++ ] = data;
                break;

            case 2 :
                mouse_buffer[ mouse_packet_index++ ] = data;
                ps2_mouse_handle_input();
                break;
        }
    }

    return 0;
}

static int ps2_mouse_start( void ) {
    int error;

    mouse_thread = create_kernel_thread(
        "ps2mouse_input",
        PRIORITY_NORMAL + 1,
        ps2_mouse_thread,
        NULL,
        0
    );

    if ( mouse_thread < 0 ) {
        return mouse_thread;
    }

    error = thread_wake_up( mouse_thread );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

input_driver_t ps2_mouse_driver = {
    .name = "PS/2 mouse",
    .type = T_MOUSE,
    .init = ps2_mouse_init,
    .destroy = NULL,
    .start = ps2_mouse_start,
    .create_state = NULL,
    .destroy_state = NULL,
    .set_state = NULL
};
