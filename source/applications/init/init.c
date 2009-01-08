/* Init application
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

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <yaosp/debug.h>

int main( int argc, char** argv ) {
    int i;

    for ( i = 0; i < 5; i++ ) {
        if ( fork() == 0 ) {
            int error;
            int slave_tty;
            char tty_path[ 128 ];

            dbprintf( "Executing shell #%d!\n", i );

            snprintf( tty_path, sizeof( tty_path ), "/device/pty/tty%d", i );

            slave_tty = open( tty_path, 0 /* O_RDWR */ );

            if ( slave_tty < 0 ) {
                dbprintf( "Failed to open tty: %s\n", tty_path );
                _exit( -1 );
            }

            dup2( slave_tty, 0 );
            dup2( slave_tty, 1 );
            dup2( slave_tty, 2 );

            error = execve( "/yaosp/SYSTEM/SHELL", NULL, NULL );
            dbprintf( "Failed to execute shell: %d\n", error );

            _exit( -1 );
        }
    }

    return 0;
}
