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
#include <stdlib.h>
#include <sys/wait.h>

#include <yaosp/debug.h>

#include <readline/readline.h>
#include <readline/history.h>

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
    pid_t child;
    char cwd[ 256 ];
    char prompt[ 256 ];

    int arg_count;
    char* child_argv[ MAX_ARGV ];

    using_history();
    rl_initialize();

    fputs( "Welcome to the yaosp shell!\nType `help' for more information.\n\n", stdout );

    while ( 1 ) {
        getcwd( cwd, sizeof( cwd ) );

        snprintf( prompt, sizeof( prompt ), "%c[1;40;32m%s %c[34m$%c[0m ", 27, cwd, 27, 27 );

        /* Read in a line */

        line = readline( prompt );

        if ( line == NULL ) {
            printf( "\n" );
            continue;
        }

        /* Skip whitespaces at the beginning of the line */

        char* tmpline = line; /* Preserve pointer for free() */

        while ( ( *line != 0 ) && ( isspace(*line) ) ) {
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

        /* Handle history */

        char* tmp;
        HISTORY_STATE* hist_state = history_get_history_state();

        if ( hist_state->length > 0 ) {
            tmp = history_get( hist_state->length )->line;
        } else {
            tmp = "";
        }

        if ( strcmp( line, tmp ) != 0 ) {
            add_history( line );
        }

        free( hist_state );

        /* Parse arguments */

        int strip_first_and_last = 0;
        int in_quote_1 = 0;
        int in_quote_2 = 0;
        size_t start = 0;
        size_t length = strlen(line);
        arg_count = 0;

        for ( i = 0; i < length; i++ ) {
            if ( line[i] == ' ' ) {
                if ( !in_quote_1 && !in_quote_2 ) {
                    line[i] = 0;
                    if ( i - start > 0 ) {
                        if ( strip_first_and_last ) {
                            child_argv[arg_count++] = strndup( line + start + 1, i - start - 2 );
                        } else {
                            child_argv[arg_count++] = strndup( line + start, i - start );
                        }
                    }
                    start = i + 1;
                    strip_first_and_last = 0;
                }
            } else if ( !in_quote_2 && line[i] == '"' ) {
                if ( !in_quote_1 ) {
                    strip_first_and_last = 1;
                }
                in_quote_1 = !in_quote_1;
            } else if ( !in_quote_1 && line[i] == '\'' ) {
                if ( !in_quote_2 ) {
                    strip_first_and_last = 1;
                }
                in_quote_2 = !in_quote_2;
            }
        }

        if ( i - start > 0 ) {
            if ( strip_first_and_last ) {
                child_argv[arg_count++] = strndup( line + start + 1, i - start - 2 );
            } else {
                child_argv[arg_count++] = strndup( line + start, i - start );
            }
        }

        /* TODO: free the strings created by strndup() later on */

        child_argv[arg_count] = NULL;

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

        free(tmpline);
    }

    return 0;
}
