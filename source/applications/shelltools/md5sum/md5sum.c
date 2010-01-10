/* md5sum shell command
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

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "md5.h"

static char* argv0 = NULL;
static unsigned char buffer[ 32768 ];

static int do_md5sum( char* filename ) {
    int i;
    int f;
    int ret;
    MD5_CTX context;
    unsigned char digest[ 16 ];

    MD5Init( &context );

    f = open( filename, O_RDONLY );

    if ( f < 0 ) {
        fprintf( stderr, "%s: failed to open %s: %s.\n", argv0, filename, strerror( errno ) );
        return -1;
    }

    do {
        ret = read( f, buffer, sizeof( buffer ) );

        if ( ret <= 0 ) {
            break;
        }

        MD5Update( &context, buffer, ret );
    } while ( ret == sizeof( buffer ) );

    close( f );

    MD5Final( digest, &context );

    for ( i = 0; i < sizeof( digest ); i++ ) {
        printf( "%02x", ( int )digest[ i ] & 0xFF );
    }

    printf( " %s\n", filename );

    return 0;
}

int main( int argc, char** argv ) {
    int i;

    argv0 = argv[ 0 ];

    for ( i = 1; i < argc; i++ ) {
        do_md5sum( argv[ i ] );
    }

    return EXIT_SUCCESS;
}
