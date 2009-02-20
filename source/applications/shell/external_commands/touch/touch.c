/* touch shell command
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
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>

#define TOUCH_ATIME 1
#define TOUCH_NOCREATE 2
#define TOUCH_MTIME 4

char* argv0 = NULL;

static void print_usage(int status){

    if( status != EXIT_SUCCESS ){ /* option error */
        fprintf(stderr, "Try `%s --help' for more information.\n", argv0);
    }else{ /* --help */
        printf("Usage: %s [OPTION]... FILE...\n", argv0);
        printf("Update the access and modification times of each FILE to the current time.\n\
\n\
A FILE argument that does not exist is created empty.\n\
\n\
A FILE argument string of - is handled specially and causes touch to\n\
change the times of the file associated with standard output.\n\
\n\
Mandatory arguments to long options are mandatory for short options too.\n\
      -a                     change only the access time\n\
      -c, --no-create        do not create any files\n\
      -d, --date=STRING      parse STRING and use it instead of current time\n\
      -m                     change only the modification time\n\
      -r, --reference=FILE   use this file's times instead of current time\n\
      -t STAMP               use [[CC]YY]MMDDhhmm[.ss] instead of current time\n\
      --time=WORD            change the specified time:\n\
                               WORD is access, atime, or use: equivalent to -a\n\
                               WORD is modify or mtime: equivalent to -m\n\
      --help                 display this help and exit\n\
\n\
Note that the -d and -t options accept different time-date formats.\n");
    }
    exit(status);
}

static char const short_options[] = "acd:t:h";

static struct option long_options[] = {
    {"no-create", no_argument, NULL, 'c'},
    {"date", required_argument, NULL, 'd'},
    {"reference", required_argument, NULL, 'r'},
    {"time", required_argument, NULL, 'U'},
    {"help", no_argument, NULL, 'h'},
/*    {"version", no_argument, NULL, 'V'}, */
    {NULL, 0, NULL, 0}
};

int do_touch(const char* file, int mode){
    int fd;

    /* TODO: check mode */

    fd = open( file, O_WRONLY | O_CREAT );

    if ( fd < 0 ) {
        fprintf( stderr, "%s: Failed to open: %s\n", argv0, file );
        return fd;
    }

    close( fd );

    return 0;
}

int main( int argc, char** argv ) {
    int error = EXIT_SUCCESS;
    int optc;
    int i;
    int mode = 0;

    argv0 = argv[ 0 ];

    /* Get the command line options */
    opterr = 0;

    for(;;){
        optc = getopt_long (argc, argv, short_options,
                       long_options, NULL);

        if (optc == -1){ /* No more options */
            break;
        }

        /* TODO */
        switch(optc){
            case 'a':
                mode |= TOUCH_ATIME;
                break;
            case 'c':
                mode |= TOUCH_NOCREATE;
                break;
            case 'd':
                mode |= TOUCH_MTIME;
                break;
            case 'r':
                break;
            case 't':
                break;
            case 'h':
                print_usage(EXIT_SUCCESS);
                break;
            /* TODO */
            default:
                print_usage(EXIT_FAILURE);
        }
    }

    /* TODO: checks */
    if (optind == argc){
        fprintf(stderr, "%s: missing operand\n", argv0);
        print_usage(EXIT_FAILURE);
    }

    for ( i = optind; i < argc; i++ ) {
        if ( do_touch( argv[ i ], mode ) < 0 ) {
            error = EXIT_FAILURE;
        }
    }


    return error;
}
