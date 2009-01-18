/* ps shell command
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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

static int read_proc_entry_node( char* dir, char* node, char* buffer, size_t size ) {
    int fd;
    int data;
    char path[ 64 ];

    snprintf( path, sizeof( path ), "/process/%s/%s", dir, node );

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

static void do_ts( char* thread, char* tid ) {
    int error;
    char name[ 128 ];

    error = read_proc_entry_node( thread, "name", name, sizeof( name ) );

    if ( error < 0 ) {
        return;
    }

    printf( "%4s %4s  `- %s\n", "", tid, name );
}

static void do_ps( char* process ) {
    int error;
    char name[ 128 ];
    char path[ 128 ];
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stat;

    error = read_proc_entry_node( process, "name", name, sizeof( name ) );

    if ( error < 0 ) {
        return;
    }

    printf( "%4s %4s %s\n", process, "-", name );

    snprintf( path, sizeof( path ), "/process/%s", process );

    dir = opendir( path );

    if ( dir == NULL ) {
        return;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        snprintf( path, sizeof( path ), "/process/%s/%s", process, entry->d_name );

        if ( stat( path, &entry_stat ) != 0 ) {
            continue;
        }

        if ( !S_ISDIR( entry_stat.st_mode ) ) {
            continue;
        }

        snprintf( path, sizeof( path ), "%s/%s", process, entry->d_name );

        do_ts( path, entry->d_name );
    }

    closedir( dir );
}

int main( int argc, char** argv ) {
    DIR* dir;
    struct dirent* entry;

    dir = opendir( "/process" );

    if ( dir == NULL ) {
        fprintf (stderr, "%s: Failed to open /process!\n", argv[0] );
        return EXIT_FAILURE;
    }

    printf( " PID  TID NAME\n" );

    while ( ( entry = readdir( dir ) ) != NULL ) {
        do_ps( entry->d_name );
    }

    closedir( dir );

    return EXIT_SUCCESS;
}
