/* Entry point of the C library
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

#include <yaosp/debug.h>

extern int init_malloc( void );
extern int main( int argc, char** argv, char** envp );

int errno;

int __libc_start_main( void ) {
    int error;

    error = init_malloc();

    if ( error < 0 ) {
        dbprintf( "Failed to initialize malloc!\n" );
        return error;
    }

    return main( 0, 0, 0 );
}
