/* execl function
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

#include <stdarg.h>
#include <unistd.h>

#define ARGV_MAX 512

int execl( const char* path, const char* arg, ... ) {
    int i;
    va_list ap;
    const char* argv[ ARGV_MAX ];

    argv[ 0 ] = arg;
    va_start( ap, arg );

    for ( i = 1; i < ARGV_MAX; i++ ) {
        argv[ i ] = va_arg( ap, const char* );

        if ( argv[ i ] == NULL ) {
            break;
        }
    }

    va_end( ap );

    return execv( path, ( char* const* )argv );
}
