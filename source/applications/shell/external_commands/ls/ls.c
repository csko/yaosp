/* ls shell command
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

int main( int argc, char** argv ) {
    DIR* dir;
    struct dirent* entry;

    dir = opendir( "." );

    if ( dir == NULL ) {
        return -1;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        fputs( entry->name, stdout );
        fputs( "\n", stdout );
    }

    closedir( dir );

    return 0;
}
