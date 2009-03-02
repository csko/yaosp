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
#include <sys/stat.h>

static char* argv0 = NULL;

int main( int argc, char** argv ) {
    int error;
    struct stat st;

    argv0 = argv[ 0 ];

    if ( argc < 4 ) {
        fprintf( stderr, "usage: %s: device path filesystem\n", argv0 );

        return EXIT_FAILURE;
    }

    if ( stat( argv[ 2 ], &st ) != 0 ) {
        fprintf( stderr, "%s: Failed to stat: %s\n", argv0, argv[ 2 ] );

        return EXIT_FAILURE;
    }

    if ( !S_ISDIR( st.st_mode ) ) {
        fprintf( stderr, "%s: %s is not a directory!\n", argv0, argv[ 2 ] );

        return EXIT_FAILURE;
    }

    error = mount(
        argv[ 1 ],
        argv[ 2 ],
        argv[ 3 ],
        MOUNT_NONE, /* TODO */
        NULL
    );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to mount %s to %s\n", argv0, argv[ 1 ], argv[ 2 ] );

        return EXIT_FAILURE;
    }

    printf( "%s is mounted to %s.\n", argv[ 1 ], argv[ 2 ] );

    return EXIT_SUCCESS;
}
