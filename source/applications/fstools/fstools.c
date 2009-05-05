/* Filesystem tools
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "fstools.h"

extern filesystem_calls_t ext2_calls;

static char* argv0;
static char* action = NULL;
static char* fstype = NULL;
static char* device = NULL;

static filesystem_calls_t* filesystem_calls[] = {
    &ext2_calls,
    NULL
};

static fstools_action_t actions[] = {
    { "create", fstools_do_create },
    { NULL, NULL }
};

static char const short_options[] = "a:f:d:h";

static struct option long_options[] = {
    { "action", required_argument, NULL, 'a' },
    { "filesystem", required_argument, NULL, 'f' },
    { "device", required_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' }
};

static void print_usage( int status ) {
    int i;

    if ( status != EXIT_SUCCESS ) { /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf( "Usage: %s --action=ACTION --filesystem=FILESYSTEM --device=DEVICE\n", argv0 );
        printf( "  -a, --action=ACTION\n" );
        printf( "      available actions:" );
        for ( i = 0; actions[ i ].name != NULL; i++ ) {
            printf( " %s", actions[ i ].name );
        }
        printf( "\n" );

        printf( "  -f, --filesystem=FILESYSTEM\n" );
        printf( "      available filesystems:" );
        for ( i = 0; filesystem_calls[ i ] != NULL; i++ ) {
            printf( " %s", filesystem_calls[ i ]->name );
        }
        printf( "\n" );

        printf( "  -d, --device=DEVICE\n" );
    }

    exit( status );
}

int fstools_do_create(){
    int i;
    filesystem_calls_t* fs_calls = NULL;

    for ( i = 0; filesystem_calls[ i ] != NULL; i++ ) {
        if ( strcmp( filesystem_calls[ i ]->name, fstype ) == 0 ) {
            fs_calls = filesystem_calls[ i ];
            break;
        }
    }

    if ( fs_calls == NULL ) {
        fprintf( stderr, "%s: Filesystem `%s' not known!\n", argv0, fstype );
        return EXIT_FAILURE;
    }

    fs_calls->create( device );

    return 0;
}

int main( int argc, char** argv ) {
    int i;
    int optc;

    argv0 = argv[ 0 ];

    if ( argc == 1 ) {
        print_usage( EXIT_FAILURE );
    }

    /* Get the command line options */

    opterr = 0;

    for ( ;; ) {
        optc = getopt_long(
            argc,
            argv,
            short_options,
            long_options,
            NULL
        );

        /* No more options? */

        if ( optc == -1 ) {
            break;
        }

        switch ( optc ) {
            case 'a' :
                action = optarg;
                break;

            case 'f' :
                fstype = optarg;
                break;

            case 'd' :
                device = optarg;
                break;

            case 'h' :
                print_usage( EXIT_SUCCESS );

            default:
                print_usage( EXIT_FAILURE );
        }
    }

    /* action, filesystem and device are required arguments */

    if ( action == NULL){
        fprintf( stderr, "Missing argument --action.\n" );
        print_usage( EXIT_FAILURE );
    }

    if( fstype == NULL ){
        fprintf( stderr, "Missing argument --filesystem.\n" );
        print_usage( EXIT_FAILURE );
    }

    if( device == NULL) {
        fprintf( stderr, "Missing argument --device.\n" );
        print_usage( EXIT_FAILURE );
    }

    /* Handle actions */

    for ( i = 0; actions[ i ].name != NULL; i++ ) {
        if ( strcmp( actions[ i ].name, action ) == 0 ) {
            return actions[ i ].function();
        }
    }

    fprintf( stderr, "%s: No such action `%s'.\n", argv0, action );

    return EXIT_FAILURE;
}
