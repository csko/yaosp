/* fread function
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
#include <string.h>
#include <unistd.h>

size_t fread( void* ptr, size_t size, size_t nmemb, FILE* stream ) {
    int res;
    unsigned long i, j;

    j = size * nmemb;
    i = 0;

    if ( ( stream->flags & __FILE_CAN_READ ) == 0 ) {
        stream->flags |= __FILE_ERROR;
        return 0;
    }

    if ( ( j == 0 ) ||
         ( ( j / nmemb ) != size ) ) {
        return 0;
    }

    if ( stream->has_ungotten ) {
        stream->has_ungotten = 0;
        *( char* )ptr = stream->unget_buffer;
        ++i;

        if ( j == 1 ) {
            return 1;
        }
    }

    for ( ; i < j; ++i ) {
        res = fgetc( stream );

        if ( res == EOF ) {
            return i / size;
        } else {
            ( ( unsigned char* )ptr )[ i ] = ( unsigned char )res;
        }
    }

    return nmemb;
}
