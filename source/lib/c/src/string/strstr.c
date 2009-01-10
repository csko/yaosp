/* strstr function
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

char* strstr( const char* s1, const char* s2 ) {
    int l1, l2;

    l2 = strlen( s2 );

    if ( !l2 ) {
        return ( char * )s1;
    }

    l1 = strlen( s1 );

    while ( l1 >= l2 ) {
        l1--;

        if ( !memcmp( s1, s2, l2 ) ) {
            return ( char * )s1;
        }

        s1++;
    }

    return NULL;
}
