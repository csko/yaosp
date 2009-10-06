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

#include <unistd.h>
#include <stdio.h>

#include <yaosp/module.h>

int main( int argc, char** argv ) {
    pid_t pid;

    /* Load the VESA graphics module */

    load_module( "vesa" );

    /* Start the guiserver */

    pid = fork();

    if ( pid == 0 ) {
        char* const argv[] = { "guiserver", NULL };

        execv( "/application/guiserver", argv );
        _exit( 0 );
    } else {
        printf( "Guiserver started with pid: #%d\n", pid );
    }

    /* Start the taskbar */

    pid = fork();

    if ( pid == 0 ) {
        char* const argv[] = { "taskbar", NULL };

        execv( "/application/taskbar/taskbar", argv );
        _exit( 0 );
    } else {
        printf( "Taskbar started with pid: #%d\n", pid );
    }

    return 0;
}
