/* strdup function
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

#include <stdlib.h>
#include <string.h>

char* strndup( const char* s, size_t n ) {
    char* s2;
    size_t len;

    len = strlen( s );
    len = MIN( len, n );

    s2 = ( char* )malloc( len + 1 );

    if ( s2 == NULL ) {
        return NULL;
    }

    memcpy( s2, s, len );
    s2[ len ] = '\0';

    return s2;
}
