/* remove shell command
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
    if ( status != EXIT_SUCCESS ){ /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf("Usage: %s [OPTION] FILE...\n", argv0);
        printf("Remove (unlink) the FILE(s).\n\
\n\
  -f, --force              ignore nonexistent files, never prompt\n\
  -i                       prompt before every removal\n\
  -I                       prompt once before removing more than three files, or\n\
                           when removing recursively.  Less intrusive than -i,\n\
                           while still giving protection against most mistakes\n\
      --interactive[=WHEN] prompt according to WHEN: never, once (-I), or\n\
                           always (-i).  Without WHEN, prompt always\n\
      --one-file-system    when removing a hierarchy recursively, skip any\n\
                           directory that is on a file system different from\n\
                           that of the corresponding command line argument\n\
      --no-preserve-root   do not treat `/' specially\n\
      --preserve-root      do not remove `/' (default)\n\
  -r, -R, --recursive      remove directories and their contents recursively\n\
  -v, --verbose            explain what is being done\n\
      --help               display this help and exit\n\
\n\
By default, rm does not remove directories.  Use the --recursive (-r or -R)\n\
option to remove each listed directory, too, along with all of its contents.\n\
\n\
To remove a file whose name starts with a `-', for example `-foo',\n\
use one of these commands:\n\
  rm -- -foo\n\
\n\
  rm ./-foo\n\
\n\
Note that if you use rm to remove a file, it is usually possible to recover\n\
the contents of that file.\n");
    }

    exit( status );
}

static char const short_options[] = "fiI::rRvh";

static struct option long_options[] = {
    { "force", no_argument, NULL, 'f' },
    { "interactive", optional_argument, NULL, 'J' },
    { "one-file-system", no_argument, NULL, 'O' },
    { "no-preserve-root", no_argument, NULL, 'N' },
    { "preserve-root", no_argument, NULL, 'P' },
    { "recursive", no_argument, NULL, 'r' },
    { "verbose", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'h' },
    /*{ "version", no_argument, NULL, 'V' },*/
    { NULL, 0, NULL, 0 }
};

int do_remove( const char* file ) {
    int error;

    error = unlink( file );

    if ( error < 0 ) {
        fprintf( stderr, "%s: cannot remove `%s': %s\n", argv0, file, strerror( errno ) );
        return error;
    }

    return 0;
}

int main( int argc, char** argv ) {
    int error = EXIT_SUCCESS;
    int i;
    int optc;

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
            case 'f':
                break;

            case 'i':
                break;

            case 'I':
                break;

            case 'r':
                break;

            case 'h':
                print_usage( EXIT_SUCCESS );
                break;
            /* TODO */

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
        if ( do_remove( argv[ i ] ) < 0 ) {
            error = EXIT_FAILURE;
        }
    }

    return error;
}
