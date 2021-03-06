/* opendir function
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

#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>

DIR* opendir( const char* name ) {
    DIR* dir;

    dir = ( DIR* )malloc( sizeof( DIR ) );

    if ( dir == NULL ) {
        return NULL;
    }

    dir->fd = open( name, O_RDONLY );

    if ( dir->fd < 0 ) {
        free( dir );
        return NULL;
    }

    return dir;
}
