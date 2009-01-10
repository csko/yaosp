/* ls shell command
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
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

char* argv0 = NULL;

int do_ls( char* dirname ) {
    DIR* dir;
    struct dirent* entry;

    struct stat entry_stat;
    char full_path[ 256 ];

    dir = opendir( dirname );

    if ( dir == NULL ) {
        printf( "%s: cannot access %s: No such file or directory\n", argv0, dirname);
        return -1;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        if ( strcmp( dirname, "/" ) == 0 ) {
            snprintf( full_path, sizeof( full_path ), "/%s", entry->name );
        } else {
            snprintf( full_path, sizeof( full_path ), "%s/%s", dirname, entry->name );
        }

        if ( stat( full_path, &entry_stat ) != 0 ) {
            continue;
        }

        if ( S_ISDIR( entry_stat.st_mode ) ) {
            printf( "directory %s\n", entry->name );
        } else {
            printf( "%9u %s\n", ( unsigned int )entry_stat.st_size, entry->name );
        }
    }

    closedir( dir );

    return 0;
}

int main( int argc, char** argv ) {
    int i;
    int ret = 0;

    if ( argc > 0 ) {
        argv0 = argv[0];
    }

    if( argc > 1 ) {
        for ( i = 1; i < argc; i++){
            if( do_ls( argv[i] ) < 0 ){
                ret = 1;
            }
        }
    } else {
        if( do_ls( "." ) < 0 ) {
            ret = 1;
        }
    }

    return ret;
}
