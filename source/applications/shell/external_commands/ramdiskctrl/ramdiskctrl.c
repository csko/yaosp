/* RAM disk controller shell command
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
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "control.h"

typedef int ctrl_action_func_t( void );

typedef struct ctrl_action {
    const char* name;
    ctrl_action_func_t* function;
} ctrl_action_t;

char* argv0;
int ramdiskctrl_device;

uint64_t size = 0;
char* file = NULL;

static char const short_options[] = "a:s:f:h";

static struct option long_options[] = {
    { "action", required_argument, NULL, 'a' },
    { "size", required_argument, NULL, 's' },
    { "file", required_argument, NULL, 'f' },
    { "help", no_argument, NULL, 'h' }
};

static void print_usage( int status ) {
    if ( status != EXIT_SUCCESS ) { /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf( "Usage: %s [OPTION]\n", argv0 );
        printf( "  -a, --action=ACTION\n" );
        printf( "  -s, --size=SIZE\n" );
        printf( "  -f, --file=FILE\n" );
        printf( "\n" );
        printf( "Creating a new ramdisk:\n" );
        printf( "  %s --action=create --size=1048576\n", argv0 );
        printf( "  %s --action=create --file=a.img\n", argv0 );
    }

    exit( status );
}

static int ramdisk_do_create( void ) {
    int error;
    ramdisk_create_info_t data;

    /* Make sure size is valid */

    if ( file == NULL ) {
        /* Create an empty ramdisk node with the given size */

        if ( size == 0 ) {
            fprintf( stderr, "%s: Size parameter is invalid/not specified!\n", argv0 );
            return EXIT_FAILURE;
        }

        data.load_from_file = 0;
    } else {
        /* Create the ramdisk node from an existing file */

        struct stat st;

        if ( stat( file, &st ) != 0 ) {
            fprintf( stderr, "%s: Failed to stat file: %s\n", argv0, file );
            return EXIT_FAILURE;
        }

        if ( ( st.st_size == 0 ) || ( !S_ISREG( st.st_mode ) ) ) {
            fprintf( stderr, "%s: Invalid file: %s\n", argv0, file );
            return EXIT_FAILURE;
        }

        data.load_from_file = 1;

        strncpy( data.image_file, file, 256 );
        data.image_file[ 255 ] = 0;

        size = st.st_size;
    }

    /* Setup the structure we need to create the new ramdisk node */

    data.size = size;

    /* Ask the ramdisk driver to create the new node */

    error = ioctl( ramdiskctrl_device, IOCTL_RAMDISK_CREATE, &data );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to create a new ramdisk!\n", argv0 );
        return EXIT_FAILURE;
    }

    printf( "New ramdisk is created at /devide/disk/%s\n", data.node_name );

    return EXIT_SUCCESS;
}

static ctrl_action_t actions[] = {
    { "create", ramdisk_do_create },
    { NULL, NULL }
};

int main( int argc, char** argv ) {
    int i;
    int optc;
    int error;
    char* action = NULL;

    argv0 = argv[ 0 ];

    if ( argc == 1 ) {
        print_usage( EXIT_FAILURE );
    }

    /* Open the ramdisk controller device */

    ramdiskctrl_device = open( "/device/control/ramdisk", O_RDONLY );

    if ( ramdiskctrl_device < 0 ) {
        fprintf( stderr, "%s: RAMDisk module not loaded!\n", argv0 );
        return EXIT_FAILURE;
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

            case 's' :
                size = atoi( optarg );
                break;

            case 'f' :
                file = optarg;
                break;

            case 'h' :
                print_usage( EXIT_SUCCESS );

            default:
                print_usage( EXIT_FAILURE );
        }
    }

    /* Action is a required argument */

    if ( action == NULL ) {
        close( ramdiskctrl_device );
        print_usage( EXIT_FAILURE );
    }

    /* Handle actions */

    error = EXIT_FAILURE;

    for ( i = 0; actions[ i ].name != NULL; i++ ) {
        if ( strcmp( actions[ i ].name, action ) == 0 ) {
            error = actions[ i ].function();
            break;
        }
    }

    close( ramdiskctrl_device );

    return error;
}
