/* ps shell command
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
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

static int read_proc_entry_node( char* dir, char* node, char* buffer, size_t size ) {
    int fd;
    int data;
    char path[ 64 ];

    snprintf( path, sizeof( path ), "/proc/%s/%s", dir, node );

    fd = open( path, O_RDONLY );

    if ( fd < 0 ) {
        return fd;
    }

    data = read( fd, buffer, size - 1 );

    close( fd );

    if ( data < 0 ) {
        return data;
    }

    buffer[ data ] = 0;

    return 0;
}

static void do_ps( char* process ) {
    int error;
    char name[ 128 ];

    error = read_proc_entry_node( process, "name", name, sizeof( name ) );

    if ( error < 0 ) {
        return;
    }

    printf( "%4s %s\n", process, name );
}

int main( int argc, char** argv ) {
    DIR* dir;
    struct dirent* entry;

    dir = opendir( "/proc" );

    if ( dir == NULL ) {
        return EXIT_FAILURE;
    }

    printf( " pid name\n" );
    printf( "-----------------------\n" );

    while ( ( entry = readdir( dir ) ) != NULL ) {
        do_ps( entry->d_name );
    }

    closedir( dir );

    return EXIT_SUCCESS;
}
