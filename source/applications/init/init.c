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
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <yaosp/debug.h>
#include <yaosp/module.h>

static int create_temp_directory( void ) {
    int error;

    error = mkdir( "/temp", 0777 );

    if ( error < 0 ) {
        return error;
    }

    error = load_module( "ramfs" );

    if ( error < 0 ) {
        return error;
    }

    error = mount( "", "/temp", "ramfs", 0, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int main( int argc, char** argv ) {
    int i;
    int f;
    pid_t child;

    /* Create symbolic links */

    symlink( "/application", "/yaosp/application" );
    symlink( "/system", "/yaosp/system" );

    /* Create and mount the /temp directory */

    create_temp_directory();

    /* Start shells */

    dbprintf( "Starting shells ...\n" );

    for ( i = 0; i < 5; i++ ) {
        child = fork();

        if ( child == 0 ) {
            int error;
            int slave_tty;
            char tty_path[ 128 ];

            char* argv[] = { "shell", NULL };
            char* envv[] = {
                "PATH=/yaosp/application:/yaosp/package/python-2.5.4",
                "HOME=/",
                "TERM=vt100",
                "TEMP=/temp",
                NULL
            };

            snprintf( tty_path, sizeof( tty_path ), "/device/terminal/tty%d", i );

            slave_tty = open( tty_path, O_RDWR );

            if ( slave_tty < 0 ) {
                dbprintf( "Failed to open tty: %s\n", tty_path );
                _exit( -1 );
            }

            dup2( slave_tty, 0 );
            dup2( slave_tty, 1 );
            dup2( slave_tty, 2 );

            close( slave_tty );

            error = execve( "/yaosp/application/shell", argv, envv );
            dbprintf( "Failed to execute shell: %d\n", error );

            _exit( -1 );
        } else if ( child < 0 ) {
            dbprintf( "Failed to create child process for shell #%d (error=%d)\n", i, child );
        }
    }

    dbprintf( "Shells started!\n" );

    /* Switch to the first terminal */

    f = open( "/device/control/terminal", O_RDONLY );

    if ( f >= 0 ) {
        int tmp = 0;

        ioctl( f, IOCTL_TERM_SET_ACTIVE, &tmp );

        close( f );
    }

    return 0;
}
