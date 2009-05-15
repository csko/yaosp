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

#include <console.h>
#include <module.h>

#include "input.h"

int init_module( void ) {
    int error;

    error = init_node_manager();

    if ( error < 0 ) {
        return error;
    }

    error = init_input_controller();

    if ( error < 0 ) {
        return error;
    }

    error = init_input_drivers();

    if ( error < 0 ) {
        return error;
    }

    kprintf( "Input: Initialized.\n" );

    return 0;
}

int destroy_module( void ) {
    return 0;
}

MODULE_OPTIONAL_DEPENDENCIES( "ps2" );
