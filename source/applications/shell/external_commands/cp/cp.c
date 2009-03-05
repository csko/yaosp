/* cp shell command
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
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
        return EXIT_FAILURE;
    }

    if ( fstat( from_fd, &from_stat ) != 0 ) {
        fprintf( stderr, "%s: Failed to stat %s.\n", argv0, from );
        close( from_fd );
        return EXIT_FAILURE;
    }

    if ( S_ISDIR( from_stat.st_mode ) ) {
        fprintf( stderr, "%s: %s is a directory.\n", argv0, from );
        close( from_fd );
        return EXIT_FAILURE;    }

    to_fd = open( to, O_WRONLY | O_CREAT );

    if ( to_fd < 0 ) {
        fprintf( stderr, "%s: Failed to open destination file: %s\n", argv0, to );
        close( from_fd );
        return EXIT_FAILURE;
    }

    while ( from_stat.st_size > 0 ) {
        int data_size;
        size_t to_copy = MIN( COPY_BUFFER_SIZE, from_stat.st_size );

        data_size = read( from_fd, copy_buffer, to_copy );

        if ( data_size < 0 ) {
            break;
        }

        write( to_fd, copy_buffer, data_size );

        from_stat.st_size -= data_size;
    }

    close( from_fd );
    close( to_fd );

    return EXIT_SUCCESS;
}

int main( int argc, char** argv ) {
    int i;
    char* to;
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
        fprintf( stderr, "%s: Failed to stat %s.\n", argv0, to );
        return EXIT_FAILURE;
    }

    if ( S_ISDIR( st.st_mode ) ) {
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

    for ( i = 1; i <= argc - 2; i++ ) {
        char* dest;

        if ( S_ISDIR( st.st_mode ) ) {
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
