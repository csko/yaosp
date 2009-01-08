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

#include <yaosp/debug.h>

int main( int argc, char** argv ) {
    int error;

    dbprintf( "Hello World from userspace!\n" );

    if ( fork() == 0 ) {
        int slave_tty;

        dbprintf( "Executing shell!\n" );

        slave_tty = open( "/device/pty/tty0", 0 );

        dbprintf( "Slave tty: %d\n", slave_tty );

        dup2( slave_tty, 0 );
        dup2( slave_tty, 1 );
        dup2( slave_tty, 2 );

        error = execve( "/yaosp/SYSTEM/SHELL", 0, 0 );
        dbprintf( "Execve returned: %d\n", error );

        _exit( -1 );
    }

    return 0;
}
