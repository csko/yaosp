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
#include <arch/io.h> /* Remove this along with kernel_reboot! */
#include <arch/interrupt.h>/* Remove this along with kernel_reboot! */

#include "../input.h"
#include "../terminal.h"
#include "../../../input/ps2/ps2kbd.h"

enum {
    L_SHIFT = 1,
    R_SHIFT = 2,
    CAPSLOCK_ON = 4,
    SCRLOCK_ON = 8,
    NUMLOCK_ON = 16
};

static int device;
static thread_id thread;

/* TODO: Should every VT have its own qualifiers instead? */
static uint32_t qualifiers = 0;

static uint8_t last_scancode = 0;

extern uint16_t keyboard_normal_map[];
extern uint16_t keyboard_shifted_map[];
extern uint16_t keyboard_escaped_map[];

static void toggle_capslock(){
    qualifiers ^= CAPSLOCK_ON;
//    ioctl( device, CAPSLOCK_TOGGLE, NULL);
}

static void toggle_numlock(){
    qualifiers ^= NUMLOCK_ON;
//    ioctl( device, NUMLOCK_TOGGLE, NULL);
}

static void toggle_scrlock(){
    qualifiers ^= SCRLOCK_ON;
//    ioctl( device, SCRLOCK_TOGGLE, NULL );
}

/* TODO: This is a temporary place for this function.
   We need to handle this better later on.
   Also, kill processes, unmount filesystems, and do the rest of the cleanups */
static void kernel_reboot(){
    int i;

    disable_interrupts();
    /* Flush keyboard */
    for( ; (( i = inb( 0x64 ) ) & 0x01) != 0 && (i & 0x02) != 0 ; );
    /* CPU RESET */
    outb( 0xFE, 0x64 );
    asm("hlt");
    for( ; ; );
}

static void ps2_keyboard_handle( uint8_t scancode ) {
    bool up;
    uint16_t key;

    if ( scancode == 0xE0 ) {
        goto done;
    }

    if ( last_scancode == 0xE0 ) {
        key = keyboard_escaped_map[ scancode ];
    } else if ( ( qualifiers & ( L_SHIFT | R_SHIFT | CAPSLOCK_ON ) ) != 0 ) {
        key = keyboard_shifted_map[ scancode ];
    } else {
        key = keyboard_normal_map[ scancode ];
    }

    up = ( ( scancode & 0x80 ) != 0 );

    switch ( key ) {
        case KEY_L_SHIFT : if ( up ) qualifiers &= ~L_SHIFT; else qualifiers |= L_SHIFT; break;
        case KEY_R_SHIFT : if ( up ) qualifiers &= ~R_SHIFT; else qualifiers |= R_SHIFT; break;
        case KEY_CAPSLOCK : if ( !up ) toggle_capslock(); break;
        case KEY_NUMLOCK : if ( !up ) toggle_numlock(); break;
        case KEY_SCRLOCK : if ( !up ) toggle_scrlock(); break;
        /* Reboot on SHIFT-DEL, for now. */
        case KEY_DELETE : if ( qualifiers & ( L_SHIFT ) ) kernel_reboot(); break;
        case KEY_F1 :
        case KEY_F2 :
        case KEY_F3 :
        case KEY_F4 :
        case KEY_F5 :
        case KEY_F6 : {
            int term_index;

            term_index = ( key >> 8 ) - 1;

            terminal_switch_to( term_index );

            break;
        }

        default :
            if ( ( key != 0 ) && ( ( key & 0xFF00 ) == 0 ) ) {
                terminal_handle_event(
                    up ? E_KEY_RELEASED : E_KEY_PRESSED,
                    key,
                    0
                );
            }

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
