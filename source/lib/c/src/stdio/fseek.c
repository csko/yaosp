/* fseek function
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

int fseeko( FILE* stream, off_t offset, int whence ) {
    fflush( stream );

    stream->buffer_pos = 0;
    stream->buffer_data_size = 0;
    stream->flags &= ~( __FILE_EOF | __FILE_ERROR );
    stream->has_ungotten = 0;

    return ( lseek( stream->fd, offset, whence ) != -1 ? 0 : -1 );
}

int fseek( FILE* stream, long offset, int whence ) {
    return fseeko( stream, ( off_t )offset, whence );
}
