/* ftell function
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
#include <unistd.h>

long ftell( FILE* stream ) {
    off_t l;

    if ( stream->flags & ( __FILE_EOF | __FILE_ERROR ) ) {
        return -1;
    }

    l = lseek( stream->fd, 0, SEEK_CUR );

    if ( l == -1 ) {
        return -1;
    }

    if ( stream->flags & __FILE_BUFINPUT ) {
        return l - ( stream->buffer_data_size - stream->buffer_pos ) - stream->has_ungotten;
    } else {
        return l + stream->buffer_pos;
    }
}
