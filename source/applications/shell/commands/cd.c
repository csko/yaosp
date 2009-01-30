/* Change directory command
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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "../command.h"

static int cd_command_function( int argc, char** argv, char** envp ) {
    int fd;
    int error;
    struct stat st;
    char* new_directory;

    /* Decide the new directory */

    if ( argc == 1 ) {
        new_directory = getenv( "HOME" );

        if ( new_directory == NULL ) {
            fprintf( stderr, "%s: Home directory not known!\n", argv[ 0 ] );
            goto error1;
        }
    } else {
        new_directory = argv[ 1 ];
    }

    /* TODO: Use $CDPATH */

    /* Open the directory */

    fd = open( new_directory, O_RDONLY );

    if ( fd < 0 ) {
        fprintf( stderr, "%s: Invalid directory: %s\n", argv[ 0 ], new_directory );
        goto error1;
    }

    /* Make sure it is a directory */

    error = fstat( fd, &st );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to stat %s\n", argv[ 0 ], new_directory );
        goto error2;
    }

    if ( !S_ISDIR( st.st_mode ) ) {
        fprintf( stderr, "%s: %s is not a directory!\n", argv[ 0 ], new_directory );
        goto error2;
    }

    /* Change the current directory to the new one */

    error = fchdir( fd );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to change current directory to %s\n", argv[ 0 ], new_directory );
        goto error2;
    }

    close( fd );

    return EXIT_SUCCESS;

error2:
    close( fd );

error1:
    return EXIT_FAILURE;
}

builtin_command_t cd_command = {
    .name = "cd",
    .description = "change the working directory",
    .command = cd_command_function
};
