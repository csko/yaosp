/* Terminal driver
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
#include <vfs/vfs.h>

#include "../input.h"

static int device;
static thread_id thread;

static uint8_t last_scancode = 0;

extern uint16_t keyboard_normal_map[];
extern uint16_t keyboard_escaped_map[];

static void ps2_keyboard_handle( uint8_t scancode ) {
    uint16_t key;

    if ( scancode == 0xE0 ) {
        goto done;
    }

    if ( last_scancode == 0xE0 ) {
        key = keyboard_escaped_map[ scancode ];
    } else {
        key = keyboard_normal_map[ scancode ];
    }

    if ( key != 0 ) {
        terminal_handle_event(
            scancode & 0x80 ? E_KEY_RELEASED : E_KEY_PRESSED,
            key,
            0
        );
    }

done:
    last_scancode = scancode;
}

static int ps2_keyboard_thread( void* arg ) {
    int data;
    uint8_t scancode;

    while ( 1 ) {
        data = pread( device, &scancode, 1, 0 );

        if ( data <= 0 ) {
            continue;
        }

        ps2_keyboard_handle( scancode );
    }

    return 0;
}

static int ps2_keyboard_init( void ) {
    device = open( "/device/input/ps2kbd", O_RDONLY );

    if ( device < 0 ) {
        return device;
    }

    thread = create_kernel_thread( "ps2kbd_input", ps2_keyboard_thread, NULL );

    if ( thread < 0 ) {
        close( device );
        return thread;
    }

    return 0;
}

static int ps2_keyboard_start( void ) {
    int error;

    error = wake_up_thread( thread );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

terminal_input_t ps2_keyboard = {
    .name = "PS/2 keyboard",
    .init = ps2_keyboard_init,
    .start = ps2_keyboard_start
};
