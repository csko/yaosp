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

static const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB" };

int do_ls( char* dirname ) {
    DIR* dir;
    struct dirent* entry;

    struct stat entry_stat;
    char full_path[ 256 ];

    int unit;
    char size[ 10 ];

    dir = opendir( dirname );

    if ( dir == NULL ) {
        fprintf( stderr, "%s: cannot access %s: No such file or directory\n", argv0, dirname);
        return -1;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        if ( strcmp( dirname, "/" ) == 0 ) {
            snprintf( full_path, sizeof( full_path ), "/%s", entry->d_name );
        } else {
            snprintf( full_path, sizeof( full_path ), "%s/%s", dirname, entry->d_name );
        }

        if ( stat( full_path, &entry_stat ) != 0 ) {
            continue;
        }

        if ( S_ISDIR( entry_stat.st_mode ) ) {
            printf( "directory %s\n", entry->d_name );
        } else {
            unit = 0;
            while ( entry_stat.st_size >= 1024 && unit < sizeof( units ) ) {
                entry_stat.st_size /= 1024;
                unit++;
            }

            snprintf( size, 10, "%lld %2s", entry_stat.st_size, units[ unit ] );
            printf( "%9s %s\n", size, entry->d_name );
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
