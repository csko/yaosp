/* execvp function
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

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

extern char* environ[];

int execvp( const char* filename, char* const argv[] ) {
    char* path;
    char* separator;
    char tmp_path[ 128 ];
    char tmp_exec[ 128 ];

    path = getenv( "PATH" );

    if ( path == NULL ) {
        return -1;
    }

    do {
        separator = strchr( path, ':' );

        if ( separator == NULL ) {
            memcpy( tmp_path, path, strlen( path ) + 1 );
        } else {
            size_t length = ( separator - path );

            memcpy( tmp_path, path, length );
            tmp_path[ length ] = 0;
        }

        snprintf( tmp_exec, sizeof( tmp_exec ), "%s/%s", tmp_path, filename );
        execve( tmp_exec, argv, environ );

        path = separator + 1;
    } while ( separator != NULL );

    return -1;
}
