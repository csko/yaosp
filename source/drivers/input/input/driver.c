/* Input driver
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

#include "input.h"

extern input_driver_t ps2_kbd_driver;
extern input_driver_t ps2_mouse_driver;

static input_driver_t* input_drivers[ INPUT_DRV_COUNT ] = {
    &ps2_kbd_driver,
    &ps2_mouse_driver
};

int set_input_driver_states( input_state_t* state ) {
    int i;
    input_driver_t* driver;

    for ( i = 0; i < INPUT_DRV_COUNT; i++ ) {
        driver = input_drivers[ i ];

        if ( ( !driver->initialized ) ||
             ( driver->set_state == NULL ) ) {
            continue;
        }

        driver->set_state( state->driver_states[ i ] );
    }

    return 0;
}

int init_input_driver_states( input_state_t* state ) {
    int i;
    int error;
    input_driver_t* driver;

    for ( i = 0; i < INPUT_DRV_COUNT; i++ ) {
        driver = input_drivers[ i ];

        if ( ( !driver->initialized ) ||
             ( driver->create_state == NULL ) ) {
            continue;
        }

        error = driver->create_state( &state->driver_states[ i ] );

        if ( error < 0 ) {
            state->driver_states[ i ] = NULL;
            continue;
        }
    }

    return 0;
}

int init_input_drivers( void ) {
    int i;
    int error;
    input_driver_t* driver;

    for ( i = 0; i < INPUT_DRV_COUNT; i++ ) {
        driver = input_drivers[ i ];

        driver->initialized = false;

        error = driver->init();

        if ( error < 0 ) {
            continue;
        }

        error = driver->start();

        if ( error < 0 ) {
            driver->destroy();
            continue;
        }

        driver->initialized = true;
    }

    return 0;
}
