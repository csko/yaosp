/* help command
 *
 * Copyright (c) 2009 Kornel Csernai
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
#include <stdlib.h>

#include "../shell.h"
#include "../command.h"

static int help_command_function( int argc, char** argv, char** envp ) {

    printf("yaOSp help\n");
    printf("Internal shell commands:\n");
    shell_print_commands();
    /* TODO: Print all applications, possibly using ls or just listing the
       application dir OR use an internal database */

    return EXIT_SUCCESS;
}

builtin_command_t help_command = {
    .name = "help",
    .description = "prints this help text",
    .command = help_command_function
};
