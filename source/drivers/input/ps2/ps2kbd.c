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

#include "ps2kbd.h"

static int ps2_keyboard_handler( int irq, void* data, registers_t* regs ) {
    int scancode = inb ( 0x60 );

    /* TODO: Handle scancode, handle LEDs, use a buffer, etc. */

    kprintf ( "PS2KBD: [0x%x]\n", scancode );

    return 0;
}

int init_module( void ) {
    int error;

    error = request_irq(1, ps2_keyboard_handler, NULL);

    if ( error < 0 ) {
        kprintf( "PS2KBD: Failed to request IRQ for keyboard.\n" );
        return error;
    }

    kprintf ( "PS2KBD: Keyboard initialized.\n" );

    return 0;
}

int destroy_module( void ) {
    return 0;
}
