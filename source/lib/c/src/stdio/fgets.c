/* fgets function
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

char* fgets( char* s, int size, FILE* stream ) {
    char* orig = s;
    int l;

    for ( l = size; l > 1; ) {
        register int c = fgetc( stream );

        if ( c == EOF ) {
            break;
        }

        *s = c;
        ++s;
        --l;

        if ( c == '\n' ) {
            break;
        }
    }

    if ( ( l == size ) || ( ferror( stream ) ) ) {
        return 0;
    }

    *s = 0;

    return orig;
}
