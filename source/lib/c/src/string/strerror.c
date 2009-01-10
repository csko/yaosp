/* strerror function
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

#include <string.h>

static char* error_strings[] = {
    "Out of memory",
    "I/O operation failed",
    "Request timed out",
    "Operation not supported",
    "No such file or directory",
    "Already exists",
    "Recource busy",
    "This is a directory",
    "Inode not found",
    "Not an executable",
    "Bad file descriptor",
    "Hardware error",
    "Out of range"
};

char* strerror( int errnum ) {
    if ( errnum < 0 ) {
        errnum = -errnum;
    }

    errnum--;

    if ( ( errnum < 0 ) ||
         ( errnum >= ( sizeof( error_strings ) / sizeof( error_strings[ 0 ] ) ) ) ) {
        return NULL;
    }

    return error_strings[ errnum ];
}
