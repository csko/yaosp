/* mount shell command
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
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <getopt.h>

static char* argv0 = NULL;

static char* device = NULL;
static char* path = NULL;
static char* fstype = NULL;

typedef struct fs_mode {
    char* name;
    uint32_t flag;
} fs_mode_t;

static char* filesystems[] = {
    "ramfs",
    "iso9660",
    "ext2",
    NULL
};

static fs_mode_t modes[] = {
    {"ro", MOUNT_RO},
    {"noatime", MOUNT_NOATIME},
    { NULL, 0 }
};

static char const short_options[] = "f:m:h";

static struct option long_options[] = {
    { "filesystem", required_argument, NULL, 'f' },
    { "mode", required_argument, NULL, 'm' },
    { "help", no_argument, NULL, 'h' }
};

static void print_usage( int status ) {
    int i;

    if ( status != EXIT_SUCCESS ) { /* option error */
        fprintf( stderr, "Try `%s --help' for more information.\n", argv0 );
    } else { /* --help */
        printf( "Usage: %s DEVICE PATH [OPTION]...\n", argv0 );
        printf( "Available options:\n" );
        printf( "  -f, --filesystem=FILESYSTEM\n" );
        printf( "      available filesystems:" );
        for ( i = 0; filesystems[i] != NULL; i++ ) {
            printf( " %s", filesystems[i] );
        }
        printf( "\n" );
        printf( "  -m, --mode=MODE[,MODE]\n" );
        printf( "      available modes:" );
        for ( i = 0; modes[i].name != NULL; i++ ) {
            printf( " %s", modes[i].name );
        }
        printf( "\n" );

    }

    exit( status );
}

static uint32_t parse_mode(char* str){
    size_t i;
    size_t len = strlen(str);
    size_t pos = 0;
    uint32_t mode = MOUNT_NONE;
    int j;
    int validmode;

    for(i = 0; i < len + 1; i++){
        if(str[i] == ',' || str[i] == '\0'){
            str[i] = '\0';
            validmode = 0;
            for(j = 0; modes[j].name != NULL; j++ ) {
                if(strcasecmp(str + pos, modes[j].name) == 0){
                    mode |= modes[j].flag;
                    validmode = 1;
                    break;
                }
            }
            if(validmode == 0){
                fprintf( stderr, "Invalid mode `%s'.\n", str + pos );
            }
            pos = i+1;
        }
    }

    return mode;
}

int main( int argc, char** argv ) {
    int error;
    struct stat st;
    int optc;
    uint32_t mode = MOUNT_NONE; /* TODO */

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

            case 'f' :
                fstype = optarg;
                break;

            case 'm' :
                if(optarg != NULL){
                    mode = parse_mode(optarg);
                }
                break;

            case 'h' :
                print_usage( EXIT_SUCCESS );

            default:
                print_usage( EXIT_FAILURE );
        }
    }

    /* path and device are required arguments */

    if ( optind + 2 > argc || optind < argc - 2 ) {
        print_usage( EXIT_FAILURE );
    }else{
      device = argv[ optind ];
      path = argv[ optind + 1 ];
    }

    if( device == NULL) {
        fprintf( stderr, "Missing argument DEVICE.\n" );
        print_usage( EXIT_FAILURE );
    }

    if ( path == NULL){
        fprintf( stderr, "Missing argument PATH.\n" );
        print_usage( EXIT_FAILURE );
    }

    if ( stat( path, &st ) != 0 ) {
        fprintf( stderr, "%s: Failed to stat: %s\n", argv0, path );

        return EXIT_FAILURE;
    }

    if ( !S_ISDIR( st.st_mode ) ) {
        fprintf( stderr, "%s: %s is not a directory!\n", argv0, path );

        return EXIT_FAILURE;
    }

    error = mount(
        device,
        path,
        fstype,
        mode,
        NULL
    );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to mount %s to %s: %s\n", argv0, device, path, strerror( errno ) );

        return EXIT_FAILURE;
    }

    printf( "%s is mounted to %s with mode 0x%x.\n", device, path, mode );

    return EXIT_SUCCESS;
}
