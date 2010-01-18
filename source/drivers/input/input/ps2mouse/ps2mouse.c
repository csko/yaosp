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

static int ps2_mouse_init( void ) {
    mouse_device = open( "/device/input/ps2/mouse", O_RDONLY );

    if ( mouse_device < 0 ) {
        return -1;
    }

    return 0;
}

static int ps2_mouse_thread( void* arg ) {
    int prev_buttons = 0;
    mouse_movement_t data;

    while ( 1 ) {
        if ( ioctl( mouse_device, IOCTL_INPUT_MOUSE_GET_MOVEMENT, &data ) != 0 ) {
            kprintf( ERROR, "ps2mouse: Failed to read data from the device.!\n" );
            break;
        }

        if ( ( data.dx != 0 ) &&
             ( data.dy != 0 ) ) {
            input_event_t event;

            event.event = E_MOUSE_MOVED;
            event.param1 = data.dx;
            event.param2 = data.dy;

            insert_input_event( &event );
        }

        int act_buttons = prev_buttons ^ data.buttons;
        prev_buttons = data.buttons;

        if ( act_buttons & PS2_BUTTON_LEFT ) {
            input_event_t event;

            event.event = ( data.buttons & PS2_BUTTON_LEFT ) ? E_MOUSE_PRESSED : E_MOUSE_RELEASED;
            event.param1 = MOUSE_BTN_LEFT;
            event.param2 = 0;

            insert_input_event( &event );
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
