/* Startgui application
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
#include <sys/ioctl.h>

static int start_gui_engine( void ) {
    int fd;
    int error;

    fd = open( "/device/control/gui", O_RDONLY );

    if ( fd < 0 ) {
        return fd;
    }

    error = ioctl( fd, IOCTL_GUI_START, NULL );

    close( fd );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int start_taskbar( void ) {
    pid_t pid;

    /* Start the taskbar */

    pid = fork();

    if ( pid == 0 ) {
        char* const argv[] = { "taskbar", NULL };

        execv( "/application/taskbar", argv );
        _exit( 0 );
    } else {
        printf( "Taskbar started with pid: #%d\n", pid );
    }

    return 0;
}

int main( int argc, char** argv ) {
    int error;

    error = start_gui_engine();

    if ( error < 0 ) {
        return EXIT_FAILURE;
    }

    error = start_taskbar();

    if ( error < 0 ) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
