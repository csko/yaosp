/* Print working directory
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

#include "../command.h"

static int pwd_command_function( int argc, char** argv, char** envp ) {
    char buffer[ 256 ];

    if ( getcwd( buffer, sizeof( buffer ) ) == NULL ) {
        printf( "%s: Failed to get CWD!\n", argv[ 0 ] );
        return 1;
    }

    printf( "%s\n", buffer );

    return 0;
}

builtin_command_t pwd_command = {
    .name = "pwd",
    .command = pwd_command_function
};
