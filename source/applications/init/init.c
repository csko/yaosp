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
#include <stdlib.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <yaosp/debug.h>
#include <yaosp/module.h>
#include <yaosp/sysinfo.h>

static int create_temp_directory( void ) {
    if ( mkdir( "/temp", 0777 ) != 0 ) {
        return -1;
    }

    if ( load_module( "ramfs" ) != 0 ) {
        return -1;
    }

    if ( mount( "", "/temp", "ramfs", 0, NULL ) != 0 ) {
        return -1;
    }

    return 0;
}

static int start_shells( void ) {
    int i;
    int f;
    pid_t child;

    dbprintf( "Starting shells ...\n" );

    for ( i = 0; i < 5; i++ ) {
        child = fork();

        if ( child == 0 ) {
            int error;
            int slave_tty;
            char tty_path[ 128 ];

            char* argv[] = { "bash", NULL };
            char* envv[] = {
                "PATH=/yaosp/application:/yaosp/package/python-2.5.4",
                "HOME=/",
                "TERM=xterm",
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

            error = execve( "/yaosp/application/bash", argv, envv );
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

static int start_gui( void ) {
    load_module( "vesa" );

    if ( fork() == 0 ) {
        char* const argv[] = { "guiserver", NULL };

        execv( "/application/guiserver", argv );
        _exit( 0 );
    }

    if ( fork() == 0 ) {
        char* const argv[] = { "taskbar", NULL };
        char* const envv[] = {
            "PATH=/yaosp/application:/yaosp/package/python-2.5.4",
            "HOME=/",
            "TERM=xterm",
            "TEMP=/temp",
            NULL
        };

        execve( "/application/taskbar/taskbar", argv, envv );
        _exit( 0 );
    }

    return 0;
}

int main( int argc, char** argv ) {
    /* Make sure this is the first time when init was executed */

    if ( ( get_process_count() != 2 ) ||
         ( getpid() != 1 ) ) {
        return EXIT_FAILURE;
    }

    /* Create symbolic links and directories */

    symlink( "/application", "/yaosp/application" );
    symlink( "/system", "/yaosp/system" );

    mkdir( "/media", 0777 );

    /* Create and mount the /temp directory */

    create_temp_directory();

    /* Start shells */

    //start_shells();
    start_gui();

    return EXIT_SUCCESS;
}
