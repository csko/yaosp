/* Config server
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

#include <stdlib.h>
#include <yaosp/debug.h>

#include <configserver/storage.h>

static char* argv0 = NULL;
static node_t* config_root = NULL;

int main( int argc, char** argv ) {
    argv0 = argv[ 0 ];

    if ( argc != 2 ) {
        dbprintf( "%s: invalid command line.\n", argv0 );
        return EXIT_FAILURE;
    }

    if ( binary_storage.load( argv[ 1 ], &config_root ) != 0 ) {
        dbprintf( "%s: failed to load configuration: %s.\n", argv0, argv[ 1 ] );
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
