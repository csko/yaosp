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

#include "ps2.h"

int init_module( void ) {
    if ( ps2_device_init() != 0 ) {
        kprintf( WARNING, "ps2: Failed to initialize devices.\n" );
        return -1;
    }

    ps2_flush();

    request_irq( PS2_KBD_IRQ, ps2_interrupt, NULL );
    request_irq( PS2_AUX_IRQ, ps2_interrupt, NULL );

    if ( ps2_controller_init() != 0 ) {
        return -1;
    }

    ps2_flush();

    if ( ps2_setup_active_multiplexing( &ps2_mux_enabled ) != 0 ) {
        return -1;
    }

    if ( ps2_mux_enabled ) {
        kprintf( WARNING, "ps2: Active multiplexing support not yet implemented!\n" );
    } else {
        ps2_device_create( &ps2_devices[ PS2_DEV_MOUSE ] );
        ps2_device_create( &ps2_devices[ PS2_DEV_KEYBOARD ] );
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
