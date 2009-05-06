/* symlink shell command
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static char* argv0;

static void print_usage( int status ) {
    if ( status != EXIT_SUCCESS ) { /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf( "Usage: %s SOURCE TARGET\n", argv0 );
    }

    exit( status );
}

static int do_symlink( const char* src, const char* dest ) {
    int error;

    error = symlink( src, dest );

    if ( error < 0 ) {
        fprintf( stderr, "Failed to create symlink. (%d)\n", errno );
    }

    return error;
}

int main( int argc, char** argv ) {
    int error;

    argv0 = argv[ 0 ];

    if ( argc != 3 ) {
        print_usage( EXIT_FAILURE );
    }

    if ( ( strcmp( argv[ 1 ], "-h" ) == 0 ) ||
         ( strcmp( argv[ 1 ], "--help" ) == 0 ) ) {
        print_usage( EXIT_SUCCESS );
    }

    error = do_symlink( argv[ 2 ], argv[ 1 ] );

    if ( error < 0 ) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
