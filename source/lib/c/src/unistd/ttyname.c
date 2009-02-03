/* ttyname and ttyname_r functions
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

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>

static char ttyname_buffer[ 128 ];

int ttyname_r( int fd, char* buf, size_t buflen ) {
    int dir;
    int error;
    int found;

    struct stat st;
    struct dirent entry;

    error = fstat( fd, &st );

    if ( error < 0 ) {
        return -1;
    }

    dir = open( "/device/pty", O_RDONLY );

    if ( dir < 0 ) {
        return -1;
    }

    found = 0;

    while ( getdents( dir, &entry, sizeof( struct dirent ) ) == 1 ) {
        if ( entry.d_ino == st.st_ino ) {
            snprintf( buf, buflen, "/device/pty/%s", entry.d_name );
            found = 1;
            break;
        }
    }

    close( dir );

    if ( !found ) {
        return -1;
    }

    return 0;
}

char* ttyname( int fd ) {
    int error;

    error = ttyname_r( fd, ttyname_buffer, sizeof( ttyname_buffer ) );

    if ( error < 0 ) {
        return NULL;
    }

    return ttyname_buffer;
}
