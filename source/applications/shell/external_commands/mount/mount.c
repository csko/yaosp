/* mount shell command
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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
#include <sys/mount.h>

int main( int argc, char** argv ) {
    int error;

    if ( argc < 4 ) {
        fprintf( stderr, "usage: %s: device path filesystem\n", argv[ 0 ] );

        return EXIT_FAILURE;
    }

    /* TODO: Check if the mount point exists and is a directory */

    error = mount(
        argv[ 1 ],
        argv[ 2 ],
        argv[ 3 ],
        0,
        NULL
    );

    if ( error < 0 ) {
        printf( "%s: Failed to mount %s to %s\n", argv[ 0 ], argv[ 1 ], argv[ 2 ] );

        return EXIT_FAILURE;
    }

    printf( "%s is mounted to %s.\n", argv[ 1 ], argv[ 2 ] );

    return EXIT_SUCCESS;
}
