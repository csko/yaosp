/* fwrite function
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
#include <errno.h>
#include <sys/types.h>

size_t fwrite( const void* ptr, size_t size, size_t nmemb, FILE* stream ) {
    ssize_t res;
    unsigned long len=size * nmemb;
    long i;

    if ( ( stream->flags & __FILE_CAN_WRITE ) == 0 ) {
        stream->flags |= __FILE_ERROR;
        return 0;
    }

    if ( ( nmemb == 0 ) ||
         ( ( len / nmemb ) != size ) ) {
        return 0;
    }

    if ( ( len > stream->buffer_size ) || ( stream->flags & __FILE_NOBUF ) ) {
        if ( fflush( stream ) ) {
            return 0;
        }

        res = write( stream->fd, ptr, len );
    } else {
        register const unsigned char* c = ptr;

        for ( i = len; i > 0; --i, ++c ) {
            if ( fputc( *c, stream ) ) {
                res = len - i;
                goto abort;
            }
        }

        res = len;
    }

    if ( res < 0 ) {
        stream->flags |= __FILE_ERROR;
        return 0;
    }

abort:
    return size ? res / size : 0;
}
