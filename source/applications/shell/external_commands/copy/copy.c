/* copy shell command
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
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/param.h>

#define COPY_BUFFER_SIZE 65536

static char* argv0;
static char copy_buffer[ COPY_BUFFER_SIZE ];

static void print_usage( int status ) {
    if ( status != EXIT_SUCCESS ) { /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf( "Usage: %s SOURCE DEST\n", argv0 );
        printf( "   or: %s SOURCEs DEST\n", argv0 );
    }

    exit( status );
}

static int do_copy( const char* from, const char* to ) {
    int from_fd;
    int to_fd;
    struct stat from_stat;

    from_fd = open( from, O_RDONLY );

    if ( from_fd < 0 ) {
        goto error1;
    }

    if ( fstat( from_fd, &from_stat ) != 0 ) {
        fprintf( stderr, "%s: Failed to stat %s.\n", argv0, from );
        goto error2;
    }

    if ( S_ISDIR( from_stat.st_mode ) ) {
        fprintf( stderr, "%s: %s is a directory.\n", argv0, from );
        goto error2;
    }

    to_fd = open( to, O_WRONLY | O_CREAT );

    if ( to_fd < 0 ) {
        fprintf( stderr, "%s: Failed to open destination file: %s (%s)\n", argv0, to, strerror( errno ) );
        goto error2;
    }

    while ( from_stat.st_size > 0 ) {
        int written;
        int data_size;
        size_t to_copy = MIN( COPY_BUFFER_SIZE, from_stat.st_size );

        data_size = read( from_fd, copy_buffer, to_copy );

        if ( data_size < 0 ) {
            printf( "%s: Failed to read data: %s (%d)\n", argv0, strerror( errno ), errno );
            goto error3;
        }

        written = write( to_fd, copy_buffer, data_size );

        if ( written < 0 ) {
            printf( "%s: Failed to write data: %s (%d)\n", argv0, strerror( errno ), errno );
            goto error3;
        }

        from_stat.st_size -= data_size;
    }

    close( from_fd );
    close( to_fd );

    return EXIT_SUCCESS;

error3:
    close( to_fd );

error2:
    close( from_fd );

error1:
    return EXIT_FAILURE;
}

int main( int argc, char** argv ) {
    int i;
    char* to;
    int to_is_dir;
    char tmp[ 256 ];
    struct stat st;

    argv0 = argv[ 0 ];

    if ( argc < 3 ) {
        if ( ( argc == 2 ) &&
             ( ( strcmp( argv[ 1 ], "--help" ) == 0 ) ||
               ( strcmp( argv[ 1 ], "-h" ) == 0 ) ) ) {
            print_usage( EXIT_SUCCESS );
        } else {
            print_usage( EXIT_FAILURE );
        }
    }

    to = argv[ argc - 1 ];

    if ( stat( to, &st ) != 0 ) {
        to_is_dir = 0;
    } else {
        to_is_dir = S_ISDIR( st.st_mode );

        if ( to_is_dir ) {
            size_t len;

            len = strlen( to );

            if ( to[ len - 1 ] == '/' ) {
                to[ len - 1 ] = 0;
            }
        } else {
            if ( argc > 3 ) {
                fprintf( stderr, "%s: %s is not a directory!\n", argv0, to );
                return EXIT_FAILURE;
            }
        }
    }

    for ( i = 1; i <= argc - 2; i++ ) {
        char* dest;

        if ( to_is_dir ) {
            char* from_filename;

            from_filename = strrchr( argv[ i ], '/' );

            if ( from_filename == NULL ) {
                from_filename = argv[ i ];
            } else {
                from_filename++;
            }

            snprintf( tmp, sizeof( tmp ), "%s/%s", to, from_filename );

            dest = tmp;
        } else {
            dest = to;
        }

        do_copy( argv[ i ], dest );
    }

    return EXIT_SUCCESS;
}
