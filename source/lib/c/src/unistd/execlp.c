/* execlp function
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

#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <alloca.h>

int execlp( const char* file, const char* arg, ... ) {
    int i;
    int n;
    va_list ap;
    va_list bak;
    char* tmp;
    char** argv;

    va_start( ap, arg );
    __va_copy( bak, ap );

    n = 2;

    while ( ( tmp = va_arg( ap, char* ) ) != NULL ) {
        ++n;
    }

    va_end( ap );

    argv = ( char** )alloca( n * sizeof( char* ) );

    if ( argv != NULL ) {
        argv[ 0 ]= ( char* )arg;

        for ( i = 0 ; i < n; ++i ) {
            argv[ i + 1 ] = va_arg( bak, char* );
        }

        va_end( bak );

        return execvp( file, argv );
    }

    va_end( bak );

    errno = ENOMEM;

    return -1;
}
