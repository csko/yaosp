/* Shell application
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#include <readline/readline.h>

#include "command.h"
#include "shell.h"

builtin_command_t quit_command = {
    .name = "quit",
    .description = "exit from the shell",
    .command = NULL
};

builtin_command_t exit_command = {
    .name = "exit",
    .description = "exit from the shell",
    .command = NULL
};

static builtin_command_t* builtin_commands[] = {
    &cd_command,
    &pwd_command,
    &help_command,
    &quit_command,
    &exit_command,
    NULL
};

void shell_print_commands( void ) {
    int i;

    for ( i = 0; builtin_commands[ i ] != NULL; i++ ) {
        printf( "%-15s %s\n", builtin_commands[ i ]->name, builtin_commands[ i ]->description );
    }
}

int main( int argc, char** argv, char** envp ) {
    int i;
    int done;
    char* line;
    char* args;
    pid_t child;
    char cwd[ 256 ];
    char prompt[ 256 ];

    char* arg;
    int arg_count;
    char* child_argv[ MAX_ARGV ];

    fputs( "Welcome to the yaosp shell!\nType `help' for more information.\n\n", stdout );

    while ( 1 ) {
        getcwd( cwd, sizeof( cwd ) );

        snprintf( prompt, sizeof( prompt ), "%c[1;40;32m%s %c[34m$%c[0m ", 27, cwd, 27, 27 );

        /* Read in a line */

        line = readline( prompt );

        /* Skip whitespaces at the beginning of the line */

        while ( ( *line != 0 ) && ( *line == ' ' ) ) {
            line++;
        }

        /* Don't try to execute zero length lines */

        if ( line[ 0 ] == 0 ) {
            continue;
        }

        if ( strcmp( line, "quit" ) == 0 || strcmp( line, "exit" ) == 0
             || line == NULL ) {
            break;
        }

        /* Parse arguments */

        args = line;
        arg_count = 0;

        while ( ( arg_count < ( MAX_ARGV - 1 ) ) &&
                ( ( arg = strchr( args, ' ' ) ) != NULL ) ) {
            child_argv[ arg_count++ ] = args;
            *arg++ = 0;
            args = arg;
        }

        child_argv[ arg_count++ ] = args;
        child_argv[ arg_count ] = NULL;

        /* First try to run it as a builtin command */

        done = 0;

        for ( i = 0; builtin_commands[ i ] != NULL; i++ ) {
            if ( strcmp( builtin_commands[ i ]->name, child_argv[ 0 ] ) == 0 ) {
                builtin_commands[ i ]->command( arg_count, child_argv, envp );
                done = 1;
                break;
            }
        }

        if ( done ) {
            continue;
        }

        /* Create a new process and execute the application */

        child = fork();

        if ( child == 0 ) {
            int error;

            /* Try to execute the command */

            error = execvp( line, child_argv );

            if ( error < 0 ) {
                printf( "Failed to execute: %s\n", line );
            }

            /* Execvp failed, exit from the child process */

            _exit( error );
        } else {
            waitpid( child, NULL, 0 );
        }
    }

    return 0;
}
