/* mkdir shell command
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
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>

#define MKDIR_PARENTS 1
#define MKDIR_VERBOSE 2

char* argv0 = NULL;

static void print_usage(int status){

    if( status != EXIT_SUCCESS ){ /* option error */
        fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
    }else{ /* --help */
        printf("Usage: %s [OPTION] DIRECTORY...\n", argv0);
        printf("Create the DIRECTORY(ies), if they do not already exist.\n\
\n\
  -m, --mode=MODE   set file mode (as in chmod), not a=rwx - umask\n\
  -p, --parents     no error if existing, make parent directories as needed\n\
  -v, --verbose     print a message for each created directory\n\
  -h, --help        display this help and exit\n");
    }
    exit(status);
}

static char const short_options[] = "m:pvh";

static struct option long_options[] = {
    {"mode", required_argument, NULL, 'm'},
    {"parents", no_argument, NULL, 'p'},
    {"verbose", no_argument, NULL, 'v'},
    {"help", no_argument, NULL, 'h'},
/*    {"version", no_argument, NULL, 'V'}, */
    {NULL, 0, NULL, 0}
};

int mode = 0777;
int flags = 0;

int check_mode(optarg){
    /* TODO */
    return 0777;
}

int do_mkdir(const char* dirname, int umask){
    int error;

    error = mkdir( dirname, umask );

    if(error < 0){
        fprintf( stderr, "%s: cannot create directory `%s': %s\n", argv0, dirname, strerror(errno) );
    }else{
        if(flags & MKDIR_VERBOSE){
            printf("%s: created directory `%s'\n", argv0, dirname);
        }
        /* TODO: MKDIR_PARENTS */
    }

    return error;
}

int main( int argc, char** argv ) {
    int error = 0;
    int optc;
    int i;
    int mode;

    argv0 = argv[0];

    /* Get the command line options */
    opterr = 0;

    for(;;){
        optc = getopt_long (argc, argv, short_options,
                       long_options, NULL);

        if (optc == -1){ /* No more options */
            break;
        }

        switch(optc){
            case 'm':
                mode = check_mode(optarg);
                if(mode < 0){
                    fprintf(stderr, "%s: invalid mode `%s'", argv[0], optarg);
                    print_usage(EXIT_FAILURE);
                }
                break;
            case 'p':
                flags |= MKDIR_PARENTS;
                break;
            case 'v':
                flags |= MKDIR_VERBOSE;
                break;
            case 'h':
                print_usage(EXIT_SUCCESS);
                break;
            default:
                print_usage(EXIT_FAILURE);
        }
    }

    if (optind == argc){
        fprintf(stderr, "%s: missing operand\n", argv0);
        print_usage(EXIT_FAILURE);
    }

    for ( i = optind; i < argc; i++ ) {
        if ( do_mkdir( argv[ i ], mode ) < 0 ) {
            error = EXIT_FAILURE;
        }
    }

    return error;
}
