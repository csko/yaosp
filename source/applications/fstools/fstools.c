/* Filesystem tools
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
#include <stdio.h>
#include <string.h>

#include "fstools.h"

extern filesystem_calls_t ext2_calls;

static char* argv0;

static filesystem_calls_t* filesystem_calls[] = {
    &ext2_calls,
    NULL
};

/* fstools create ext2 device */

int main( int argc, char** argv ) {
    int i;
    char* fs;
    filesystem_calls_t* fs_calls;

    argv0 = argv[ 0 ];

    if ( argc < 4 ) {
        return EXIT_FAILURE;
    }

    fs = argv[ 2 ];
    fs_calls = NULL;

    for ( i = 0; filesystem_calls[ i ] != NULL; i++ ) {
        if ( strcmp( filesystem_calls[ i ]->name, fs ) == 0 ) {
            fs_calls = filesystem_calls[ i ];
            break;
        }
    }

    if ( fs_calls == NULL ) {
        fprintf( stderr, "%s: Filesystem %s not known!\n", argv0, fs );
        return EXIT_FAILURE;
    }

    fs_calls->create( argv[ 3 ] );

    return EXIT_SUCCESS;
}
