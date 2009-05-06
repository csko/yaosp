/* remdir shell command
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

static char* argv0 = NULL;

static void print_usage( int status ) {
    if ( status != EXIT_SUCCESS ) { /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf("Usage: %s [OPTION] FILE...\n", argv0);
        printf("Usage: rmdir [OPTION]... DIRECTORY...\n\
Remove the DIRECTORY(ies), if they are empty.\n\
\n\
      --ignore-fail-on-non-empty\n\
                  ignore each failure that is solely because a directory\n\
                  is non-empty\n\
  -p, --parents   Remove DIRECTORY and its ancestors.  E.g., `rmdir -p a/b/c' is\n\
                  similar to `rmdir a/b/c a/b a'.\n\
  -v, --verbose   output a diagnostic for every directory processed\n\
      --help     display this help and exit\n");

    }

    exit( status );
}

static char const short_options[] = "pvh";

static struct option long_options[] = {
    { "ignore-fail-on-non-empty", no_argument, NULL, 'I' },
    { "parents", optional_argument, NULL, 'p' },
    { "verbose", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'h' },
    /*{ "version", no_argument, NULL, 'V' },*/
    { NULL, 0, NULL, 0 }
};

int do_remdir( const char* dir ) {
    int error;

    error = rmdir( dir );

    if ( error < 0 ) {
        fprintf( stderr, "%s: cannot remove `%s': %s\n", argv0, dir, strerror( errno ) );
        return error;
    }

    return 0;
}

int main( int argc, char** argv ) {
    int error = EXIT_SUCCESS;
    int optc, i;

    argv0 = argv[ 0 ];

    /* Get the command line options */
    opterr = 0;

    for ( ;; ) {
        optc = getopt_long( argc, argv, short_options,
                       long_options, NULL );

        if ( optc == -1 ) {
            /* No more options */
            break;
        }

        switch ( optc ) {
            /* TODO */
            case 'I':
                break;

            case 'p':
                break;

            case 'v':
                break;

            case 'h':
                print_usage( EXIT_SUCCESS );
                break;

            default:
                print_usage( EXIT_FAILURE );
                break;
        }
    }


    if ( optind == argc ) {
        fprintf( stderr, "%s: missing operand\n", argv0 );
        print_usage( EXIT_FAILURE );
    }

    for ( i = optind; i < argc; i++ ) {
        if ( do_remdir( argv[ i ] ) < 0 ) {
            error = EXIT_FAILURE;
        }
    }

    return error;
}
