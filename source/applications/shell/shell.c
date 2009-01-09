/* Shell application
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#include <yaosp/debug.h>

static char line_buf[ 255 ];

static char* get_line( void ) {
    int c;
    int pos = 0;
    int done = 0;

    while ( !done ) {
        c = fgetc( stdin );

        switch ( c ) {
            case '\n' :
                done = 1;
                break;

            case '\b' :
                if ( pos > 0 ) {
                    pos--;
                }

                break;

            default :
                if ( pos < 254 ) {
                    line_buf[ pos++ ] = c;
                }

                break;
        }
    }

    line_buf[ pos ] = 0;

    return line_buf;
}

#define MAX_ARGV 32

int main( int argc, char** argv ) {
    char* line;
    char* args;
    pid_t child;

    char* arg;
    int arg_count;
    char* child_argv[ MAX_ARGV ];

    fputs( "Welcome to the yaosp shell!\n\n", stdout );

    while ( 1 ) {
        fputs( "> ", stdout );

        line = get_line();

        args = strchr( line, ' ' );

        if ( args != NULL ) {
            *args++ = 0;
        }

        /* Build the arg list */

        child_argv[ 0 ] = line;
        arg_count = 1;

        if ( args != NULL ) {
            while ( ( arg_count < ( MAX_ARGV - 2 ) ) &&
                    ( ( arg = strchr( args, ' ' ) ) != NULL ) ) {
                child_argv[ arg_count++ ] = args;
                *arg++ = 0;
                args = arg;
            }

            child_argv[ arg_count++ ] = args;
        }

        child_argv[ arg_count ] = NULL;

        /* Create a new process and execute the application */

        child = fork();

        if ( child == 0 ) {
            int error;

            error = execve( line, child_argv, NULL );

            if ( error < 0 ) {
                printf( "Failed to execute: %s\n", line );
            }

            _exit( error );
        } else {
            waitpid( child, NULL, 0 );
        }
    }

    return 0;
}
