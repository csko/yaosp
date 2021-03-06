/* strcasecmp function
 *
 * Copyright (c) 2009 Kornel Csernai
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
#include <ctype.h>

int strcasecmp( const char* s1, const char* s2 ) {
    int result;

    while ( 1 ) {
        result = tolower( *s1 ) - tolower( *s2++ );

        if ( ( result != 0 ) || ( *s1++ == 0 ) ) {
            break;
        }
    }

    return result;
}
