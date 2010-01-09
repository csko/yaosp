/* Filesystem tester application
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
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static char* argv0 = NULL;
static char* root_directory = NULL;

static char test_filename[ 256 ];

static int test_file_write( void ) {
    int f;
    int ret;
    struct stat st;

    /* Open the file */

    f = open( test_filename, O_WRONLY | O_CREAT | O_TRUNC );
    assert( f >= 0 );

    /* Write 5 bytes into the file */

    ret = write( f, "Hello", 5 );
    assert( ret == 5 );

    /* Make sure it's written properly */

    ret = fstat( f, &st );
    assert( ret == 0 );
    assert( st.st_size == 5 );

    /* Write some data again */

    ret = write( f, " world", 6 );
    assert( ret == 6 );

    /* Check the file size again */

    ret = fstat( f, &st );
    assert( ret == 0 );
    assert( st.st_size == 11 );

    /* Close the file */

    ret = close( f );
    assert( ret == 0 );

    return 0;
}

int main( int argc, char** argv ) {
    if ( argc != 2 ) {
        fprintf( stderr, "%s root_directory\n", argv[ 0 ] );
        return EXIT_FAILURE;
    }

    argv0 = argv[ 0 ];
    root_directory = argv[ 1 ];

    snprintf( test_filename, sizeof( test_filename ), "%s/test_file", root_directory );

    test_file_write();

    return EXIT_SUCCESS;
}
