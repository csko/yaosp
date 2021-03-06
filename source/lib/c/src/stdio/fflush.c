/* fflush function
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

int fflush( FILE* stream ) {
    if ( stream->flags & __FILE_BUFINPUT ) {
        int tmp;

        tmp = ( int )stream->buffer_pos - ( int )stream->buffer_data_size;

        if ( tmp != 0 ) {
            lseek( stream->fd, tmp, SEEK_CUR );
        }

        stream->buffer_pos = 0;
        stream->buffer_data_size = 0;
    } else {
        if ( stream->buffer_pos > 0 ) {
            if ( write( stream->fd, stream->buffer, stream->buffer_pos ) != stream->buffer_pos ) {
                stream->flags |= __FILE_ERROR;
                return -1;
            }

            stream->buffer_pos = 0;
        }
    }

    return 0;
}
