/* cat shell command
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFLEN 4096

static char* argv0;
static char buffer[ BUFLEN ];

/* Simple cat function */

int do_cat( char* file ) {
    struct stat st;
    int error;
    ssize_t last_read;
    error = stat( file, &st );

    if ( error != 0 ) {
        fprintf( stderr, "%s: %s: %s\n", argv0, file, strerror( errno ) );
        return -1;
    }

    if ( S_ISDIR( st.st_mode ) ) {
        fprintf( stderr, "%s: %s: Is a directory\n", argv0, file);
        return -1;
    }

    error = open( file, O_RDONLY );

    if ( error < 0 ) {
        fprintf( stderr, "%s: %s: %s.\n", argv0, file, strerror( errno ) );
        return -1;
    }

    for ( ;; ) {
        last_read = read( error, buffer, BUFLEN );

        if ( last_read <= 0 ) {
            break;
        }

        write( 1, buffer, last_read );

        if ( last_read != BUFLEN ) {
            break;
        }
    }

    close( error );

    return 0;
}

int main( int argc, char** argv ) {
    int i;
    int ret = EXIT_SUCCESS;

    argv0 = argv[ 0 ];

    if ( argc > 1 ) {
        for ( i = 1; i < argc; i++ ) {
            if ( do_cat( argv[ i ] ) < 0 ) {
                ret = EXIT_FAILURE;
            }
        }
    } else {
        /* Basic echo from stdin */

        int last_read;
    
        for ( ;; ) {
            last_read = getchar();

            if ( last_read == EOF ) {
                break;
            }

            if ( putchar( last_read ) == EOF ) {
                break;
            }
        }
    }

    return ret;
}
