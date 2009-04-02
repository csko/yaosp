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

#include <yutil/array.h>

#include <windowmanager.h>
#include <graphicsdriver.h>

static array_t window_stack;

int wm_register_window( window_t* window ) {
    int error;

    error = array_insert_item( &window_stack, 0, window );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int init_windowmanager( void ) {
    int error;

    error = init_array( &window_stack );

    if ( error < 0 ) {
        return error;
    }

    array_set_realloc_size( &window_stack, 32 );

    return 0;
}
