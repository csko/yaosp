/* fgetc function
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
#include <unistd.h>

int fgetc( FILE* stream ) {
    /* Check if we can read from the stream */

    if ( ( stream->flags & __FILE_CAN_READ ) == 0 ) {
        stream->flags |= __FILE_ERROR;
        return EOF;
    }

    /* Check the unget buffer */

    if ( stream->has_ungotten ) {
        stream->has_ungotten = 0;
        return stream->unget_buffer;
    }

    /* Check the end of the file */

    if ( feof( stream ) ) {
        return EOF;
    }

    /* Fill the buffer if it's empty */

    if ( stream->buffer_pos >= stream->buffer_data_size ) {
        ssize_t length;

        length = read( stream->fd, stream->buffer, stream->buffer_size );

        if ( length == 0 ) {
            stream->flags |= __FILE_EOF;
            return EOF;
        } else if ( length < 0 ) {
            stream->flags |= __FILE_ERROR;
            return EOF;
        }

        stream->buffer_pos = 0;
        stream->buffer_data_size = length;
    }

    /* Get one character from the buffer */

    return stream->buffer[ stream->buffer_pos++ ];
}
