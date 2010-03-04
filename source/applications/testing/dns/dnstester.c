/* DNS resolv tester application
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

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>

static char* argv0 = NULL;
static char* domain = NULL;

int main( int argc, char** argv ) {
    if ( argc != 2 ) {
        fprintf( stderr, "%s domain\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    argv0 = argv[0];
    domain = argv[1];

    gethostbyname(domain);

    return EXIT_SUCCESS;
}
