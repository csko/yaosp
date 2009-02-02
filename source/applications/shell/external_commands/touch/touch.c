/* touch shell command
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
#include <fcntl.h>
#include <unistd.h>

char* argv0 = NULL;

int main( int argc, char** argv ) {
    int fd;

    argv0 = argv[ 0 ];

    if ( argc < 2 ) {
        fprintf( stderr, "%s file\n", argv0 );
        return EXIT_FAILURE;
    }

    fd = open( argv[ 1 ], O_WRONLY | O_CREAT );

    if ( fd < 0 ) {
        fprintf( stderr, "%s: Failed to open: %s\n", argv0, argv[ 1 ] );
        return EXIT_SUCCESS;
    }

    close( fd );

    return EXIT_SUCCESS;
}
