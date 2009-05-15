/* PS/2 keyboard input driver
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

enum {
    L_SHIFT = 1,
    R_SHIFT = 2,
    CAPSLOCK = 4,
    SCRLOCK = 8,
    NUMLOCK = 16,
    L_CTRL = 32,
    R_CTRL = 64,
    L_ALT = 128,
    R_ALT = 256,
};

typedef struct ps2_kbd_state {
    uint32_t qualifiers;
} ps2_kbd_state_t;

static int device;
static thread_id thread;

static uint8_t last_scancode = 0;

static ps2_kbd_state_t* current_state = NULL;

extern uint16_t keyboard_normal_map[];
extern uint16_t keyboard_shifted_map[];
extern uint16_t keyboard_ctrl_map[];
extern uint16_t keyboard_escaped_map[];

static void ps2_keyboard_handle( uint8_t scancode ) {
    bool up;
    uint16_t key;
    input_event_t event;

    if ( ( scancode == 0xE0 ) ||
         ( current_state == NULL ) ) {
        goto done;
    }

    if ( last_scancode == 0xE0 ) {
        key = keyboard_escaped_map[ scancode & 0x7F ];
    } else if ( ( current_state->qualifiers & ( L_SHIFT | R_SHIFT ) ) != 0 ) {
        if ( current_state->qualifiers & CAPSLOCK ) {
            key = keyboard_normal_map[ scancode & 0x7F ];
        } else {
            key = keyboard_shifted_map[ scancode & 0x7F ];
        }
    } else if ( ( current_state->qualifiers & ( CAPSLOCK ) ) != 0 ) {
        if ( current_state->qualifiers & ( L_SHIFT | R_SHIFT ) ) {
            key = keyboard_normal_map[ scancode & 0x7F ];
        } else {
            key = keyboard_shifted_map[ scancode & 0x7F ];
        }
    } else if ( ( current_state->qualifiers & ( L_CTRL | R_CTRL ) ) != 0 ) {
        key = keyboard_ctrl_map[ scancode & 0x7F ];
    } else {
        key = keyboard_normal_map[ scancode & 0x7F ];
    }

    if ( key == 0 ) {
        goto done;
    }

    up = ( ( scancode & 0x80 ) != 0 );

    switch ( key ) {
        case KEY_L_SHIFT : if ( up ) current_state->qualifiers &= ~L_SHIFT; else current_state->qualifiers |= L_SHIFT; break;
        case KEY_R_SHIFT : if ( up ) current_state->qualifiers &= ~R_SHIFT; else current_state->qualifiers |= R_SHIFT; break;
        case KEY_L_CTRL : if ( up ) current_state->qualifiers &= ~L_CTRL; else current_state->qualifiers |= L_CTRL; break;
        case KEY_R_CTRL : if ( up ) current_state->qualifiers &= ~R_CTRL; else current_state->qualifiers |= R_CTRL; break;
        case KEY_L_ALT : if ( up ) current_state->qualifiers &= ~L_ALT; else current_state->qualifiers |= L_ALT; break;
        case KEY_R_ALT : if ( up ) current_state->qualifiers &= ~R_ALT; else current_state->qualifiers |= R_ALT; break;
        default :
            event.event = up ? E_KEY_RELEASED : E_KEY_PRESSED;
            event.param1 = ( int )key;

            insert_input_event( &event );

            break;
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
    int error;

    device = open( "/device/input/ps2kbd", O_RDONLY );

    if ( device < 0 ) {
        error = device;
        goto error1;
    }

    thread = create_kernel_thread(
        "ps2kbd_input",
        PRIORITY_NORMAL + 1,
        ps2_keyboard_thread,
        NULL,
        0
    );

    if ( thread < 0 ) {
        error = thread;
        goto error2;
    }

    return 0;

error2:
    close( device );
    device = -1;

error1:
    return error;
}

static void ps2_keyboard_destroy( void ) {
    /* TODO */
}

static int ps2_keyboard_start( void ) {
    int error;

    if ( ( thread < 0 ) ||
         ( device < 0 ) ) {
        return -EINVAL;
    }

    error = wake_up_thread( thread );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int ps2_keyboard_create_state( void** _state ) {
    ps2_kbd_state_t* state;

    state = ( ps2_kbd_state_t* )kmalloc( sizeof( ps2_kbd_state_t ) );

    if ( state == NULL ) {
        return -ENOMEM;
    }

    state->qualifiers = 0;

    *_state = state;

    return 0;
}

static void ps2_keyboard_destroy_state( void* state ) {
    kfree( state );
}

static int ps2_keyboard_set_state( void* state ) {
    current_state = ( ps2_kbd_state_t* )state;

    return 0;
}

input_driver_t ps2_kbd_driver = {
    .name = "PS/2 keyboard",
    .type = T_KEYBOARD,
    .init = ps2_keyboard_init,
    .destroy = ps2_keyboard_destroy,
    .start = ps2_keyboard_start,
    .create_state = ps2_keyboard_create_state,
    .destroy_state = ps2_keyboard_destroy_state,
    .set_state = ps2_keyboard_set_state
};
