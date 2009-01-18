/* date shell command
 *
 * Copyright (c) 2009 Kornel Csernai
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
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int main( int argc, char* argv[] ) {
    time_t now = time( NULL );
    char datestr[100];
    struct tm* tmval;

    if( now == (time_t) -1 ) {
        /* TODO: errno */
        fprintf ( stderr, "%s: time() failed.\n", argv[0] );
        return EXIT_FAILURE;
    }

    tmval = malloc( sizeof( struct tm ) );
    gmtime_r( &now, tmval );

    if( tmval == NULL ) {
        /* TODO: errno */
        fprintf ( stderr, "%s: gmtime_r() failed.\n", argv[0] );
        free( tmval );
        return EXIT_FAILURE;
    }

    strftime( datestr, 100, "%c", tmval );
    free( tmval );
    printf( "%s\n", datestr );

    return EXIT_SUCCESS;
}
