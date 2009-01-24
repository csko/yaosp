/* mkdir shell command
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
#include <sys/stat.h>

char* argv0;

int main( int argc, char** argv ) {
    int error;

    argv0 = argv[ 0 ];

    if ( argc == 1 ) {
        printf( "%s directory\n", argv0 );

        return EXIT_FAILURE;
    }

    error = mkdir( argv[ 1 ], 0666 );

    if ( error < 0 ) {
        printf( "%s: Failed to create directory!\n" );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
