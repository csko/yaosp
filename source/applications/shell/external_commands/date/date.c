/* date shell command, inspired by GNU coreutils' date.c
 *
 * Copyright (C) 1989-2008 Free Software Foundation, Inc.
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
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>

#define bool int
#define true 1
#define false 0

static char* argv0;

void print_usage( int status ) {

    if ( status != EXIT_SUCCESS ) { /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf( "Usage: %s [OPTION]... [+FORMAT]\n\
  or:  %s [-u|--utc|--universal] [MMDDhhmm[[CC]YY][.ss]]\n", argv0, argv0 );
        printf("Display the current time, or set the system date.\n\
\n\
  -d, --date=STRING         display time described by STRING, not `now'\n\
  -f, --file=DATEFILE       like --date once for each line of DATEFILE\n\
  -r, --reference=FILE      display the last modification time of FILE\n\
  -R, --rfc-2822            output date and time in RFC 2822 format.\n\
                            Example: Mon, 07 Aug 2006 12:34:56 -0600\n\
      --rfc-3339=TIMESPEC   output date and time in RFC 3339 format.\n\
                            TIMESPEC=`date', `seconds', or `ns' for\n\
                            date and time to the indicated precision.\n\
                            Date and time components are separated by\n\
                            a single space: 2006-08-07 12:34:56-06:00\n\
  -s, --set=STRING          set time described by STRING\n\
  -u, --utc, --universal    print or set Coordinated Universal Time\n\
  -h, --help                display this help and exit\n" );
    }

    exit( status );
}

/* A format suitable for Internet RFC 2822.  */
static char const rfc_2822_format[] = "%a, %d %b %Y %H:%M:%S %z";

static char const rfc_3339_format[][ 32 ] = {
    "%Y-%m-%d",
    "%Y-%m-%d %H:%M:%S%:z", /* TODO: strftime() support */
    "%Y-%m-%d %H:%M:%S.%N%:z" /* TODO: strftime() support */
};

static char const iso_8601_format[][ 32 ] = {
    "%Y-%m-%d",
    "%Y-%m-%dT%H:%M:%S%z",
    "%Y-%m-%dT%H:%M:%S,%N%z",
    "%Y-%m-%dT%H%z",
    "%Y-%m-%dT%H:%M%z"
};

static char const *const time_spec_string[] = {
    /* Put "hours" and "minutes" first, since they aren't valid for
       --rfc-3339.  */
    "hours", "minutes",
    "date", "seconds", "ns"
};

static char const short_options[] = "d:f::r:Rs:uh";

static struct option long_options[] = {
    { "date", required_argument, NULL, 'd' },
    { "file", required_argument, NULL, 'f' },
    { "iso-8601", optional_argument, NULL, 'I' },
    { "reference", required_argument, NULL, 'r' },
    { "rfc-822", no_argument, NULL, 'R' },
    { "rfc-2822", no_argument, NULL, 'R' },
    { "rfc-3339", required_argument, NULL, 'q' }, /* q is a random non-used character here */
    { "set", required_argument, NULL, 's' },
    { "utc", no_argument, NULL, 'u' },
    { "uct", no_argument, NULL, 'u' },
    { "universal", no_argument, NULL, 'u' },
    { "help", no_argument, NULL, 'h' },
    /*{"version", no_argument, NULL, 'V'},*/
    { NULL, 0, NULL, 0 }
};

int main( int argc, char* argv[] ) {
    time_t now;
    struct tm* tmval;
    struct stat refstats;
    const char* datestr = NULL;
    char tmpstr[ 128 ];
    const char* set_datestr = NULL;
    bool set_date = false;
    char const* format = NULL;
    char* batch_file = NULL;
    char* reference = NULL;
    bool ok;
    int option_specified_date;
    int optc;
    int i;

    argv0 = argv[ 0 ];

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

        if ( optc == -1 ) {
            /* No more options */
            break;
        }

        char const * new_format = NULL;

        switch ( optc ) {
            case 'd':
                datestr = optarg;
                break;

            case 'f':
                batch_file = optarg;
                break;

            case 'r':
                reference = optarg;
                break;

            case 'I':
                ok = false;

                for ( i = 0; i < 5; i++ ) {
                    if ( strcmp( time_spec_string[ i ], optarg ) == 0 ) {
                        new_format = iso_8601_format[ i ];
                        ok = true;
                    }
                }

                if ( !ok ) {
                    fprintf(
                        stderr,
                        "%s: invalid argument `%s' for `--iso-8601'\n",
                        argv0,
                        optarg
                    );
                    fprintf( stderr, "Valid arguments are:\n\
  - `hours'\n\
  - `minutes'\n\
  - `date'\n\
  - `seconds'\n\
  - `ns'\n" );
                    print_usage( EXIT_FAILURE );
                }

                break;

            case 'q':
                ok = false;

                for ( i = 2; i < 5; i++ ) {
                    if ( strcmp( time_spec_string[ i ], optarg ) == 0 ) {
                        new_format = rfc_3339_format[ i ];
                        ok = true;
                    }
                }

                if ( !ok ) {
                    fprintf(
                        stderr,
                        "%s: invalid argument `%s' for `--rfc-3339'\n",
                        argv0,
                        optarg
                    );
                    fprintf( stderr, "Valid arguments are:\n\
  - `date'\n\
  - `seconds'\n\
  - `ns'\n");
                    print_usage( EXIT_FAILURE );
                }

                break;

            case 'R':
                new_format = rfc_2822_format;
                break;

            case 's':
                set_datestr = optarg;
                set_date = true;

            case 'u':
                break;

            case 'h':
/*            case 'v':*/
                print_usage( EXIT_SUCCESS );
                break;

            default:
                print_usage( EXIT_FAILURE );
                break;
        }

        if ( new_format ) {
            if ( format ) {
                fprintf( stderr, "multiple output formats specified\n" );
            }

            format = new_format;
        }
    }

    /* Option checks */

    /* Only one of these can be set */
    option_specified_date = ((datestr ? 1 : 0)
               + (batch_file ? 1 : 0)
               + (reference ? 1 : 0));

    if ( option_specified_date > 1 ) {
        fprintf( stderr, "the options to specify dates for printing are mutually exclusive\n" );
        print_usage( EXIT_FAILURE );
    }

    if ( set_date && option_specified_date ) {
        fprintf( stderr, "the options to print and set the time may not be used together\n" );
        print_usage( EXIT_FAILURE );
    }

    if ( optind < argc ) {
        if ( optind + 1 < argc ) {
            fprintf( stderr, "extra operand `%s'\n", argv[ optind + 1 ] );
            print_usage( EXIT_FAILURE );
        }

        if ( argv[ optind ][ 0 ] == '+' ) {
            if ( format ) {
                fprintf( stderr, "multiple output formats specified\n" );
            }

            format = argv[ optind++ ] + 1;
        } else if ( set_date || option_specified_date ) {
            fprintf( stderr, "the argument `%s' lacks a leading `+';\n"
                    "When using an option to specify date(s), any non-option\n"
                    "argument must be a format string beginning with `+'.",
                    argv[ optind ] );
            print_usage( EXIT_FAILURE );
        }
    }

    if ( !format ) {
        format = "%a %b %e %H:%M:%S %Z %Y";
    }

    if ( batch_file != NULL ) {
        /* TODO */
        return EXIT_SUCCESS;
    } else {
        bool valid_date = true;
        ok = true;

        if ( !option_specified_date && !set_date ) {
            if ( optind < argc ) {
                /* Prepare to set system clock to the specified date/time
                   given in the POSIX-format. */
                set_date = true;
                datestr = argv[optind];
                /* TODO: read datestr to now */
                valid_date = false;
            } else {
                /* Prepare to print the current date/time. */
                now  = time( NULL );
            }
        } else {
            /* (option_specified_date || set_date) */
            if ( reference != NULL ) {
                if ( stat( reference, &refstats ) != 0 ) {
                    fprintf( stderr, "%s: %s: %s\n", argv[0], reference, strerror( errno ) );
                    return EXIT_FAILURE;
                }

                now = refstats.st_mtime;
            } else {
                if ( set_datestr ) {
                    datestr = set_datestr;
                    /* TODO: read datestr to now */
                    valid_date = false;
                }
            }
        }

        if ( valid_date == false ) {
            fprintf( stderr, "invalid date `%s'\n", datestr );
            print_usage( EXIT_FAILURE );
        }

        if ( set_date ) {
            /* Set the system clock to the specified date, then regardless of
             the success of that operation, format and print that date. */

            if ( stime( &now ) != 0 ) {
                fprintf( stderr, "cannot set time\n" );
                ok = false;
            }
        }
    }

    if ( now == ( time_t )-1 ) {
        fprintf( stderr, "%s: time() failed: %s\n", argv[0], strerror( errno ) );
        return EXIT_FAILURE;
    }

    tmval = ( struct tm* )malloc( sizeof( struct tm ) );
    gmtime_r( &now, tmval );

    if ( tmval == NULL ) {
        fprintf( stderr, "%s: gmtime_r() failed: %s\n", argv[0], strerror( errno ) );
        free( tmval );

        return EXIT_FAILURE;
    }

    strftime( ( char* )tmpstr, 128, format, tmval );
    free( tmval );
    printf( "%s\n", tmpstr );

    return EXIT_SUCCESS;
}
