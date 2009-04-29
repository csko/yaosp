/* readdir function
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
#include <unistd.h>
#include <errno.h>

struct dirent* readdir( DIR* dir ) {
    int error;

    if ( dir == NULL ) {
        return NULL;
    }

    error = getdents( dir->fd, &dir->entry, sizeof( struct dirent ) );

    if ( error == 0 ) {
        return NULL;
    }

    return &dir->entry;
}

int readdir_r( DIR* dir, struct dirent* entry, struct dirent** result ) {
    int error;

    if ( ( dir == NULL ) ||
         ( entry == NULL ) ) {
        errno = -EINVAL;
        return -1;
    }

    error = getdents( dir->fd, entry, sizeof( struct dirent ) );

    if ( error == 0 ) {
        *result = NULL;
    } else {
        *result = entry;
    }

    return 0;
}
