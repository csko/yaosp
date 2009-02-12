/* getenv function
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

#include <stdlib.h>
#include <string.h>

extern char** environ;

char* getenv( const char* name ) {
    int i;
    size_t length;
    size_t name_length;

    name_length = strlen( name );

    for ( i = 0; environ[ i ] != NULL; i++ ) {
        length = strlen( environ[ i ] );

        if ( length < ( name_length + 1 ) ) {
            continue;
        }

        if ( ( strncmp( environ[ i ], name, name_length ) == 0 ) &&
             ( environ[ i ][ name_length ] == '=' ) ) {
            return &environ[ i ][ name_length + 1 ];
        }
    }

    return NULL;
}
