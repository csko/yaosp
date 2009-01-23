/* strtok_r function
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

char* strtok_r( char* s, const char* delim, char** ptrptr ) {
    char* tmp = NULL;

    if ( s == NULL ) {
        s = *ptrptr;
    }

    s += strspn( s, delim );

    if ( *s ) {
        tmp = s;
        s += strcspn( s, delim );

        if ( *s ) {
            *s++ = 0;
        }
    }

    *ptrptr = s;

    return tmp;
}
