/* GUI server
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

#include <stdlib.h>
#include <yaosp/debug.h>

#include <input.h>

static input_driver_t* input_drivers[] = {
    &ps2mouse_driver,
    NULL
};

int init_input_system( void ) {
    int i;
    int error;
    input_driver_t* input_driver;

    for ( i = 0; input_drivers[ i ] != NULL; i++ ) {
        input_driver = input_drivers[ i ];

        error = input_driver->init();

        if ( error < 0 ) {
            dbprintf( "Failed to initialize input driver: %s\n", input_driver->name );
            continue;
        }

        error = input_driver->start();

        if ( error < 0 ) {
            dbprintf( "Failed to start input driver: %s\n", input_driver->name );
            continue;
        }

        dbprintf( "Input driver %s started.\n", input_driver->name );
    }

    return 0;
}
