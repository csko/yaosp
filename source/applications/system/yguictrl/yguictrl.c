/* yaOSp GUI control application
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
#include <stdlib.h>

#include <ygui/desktop.h>
#include <ygui/application.h>

#include "yguictrl.h"

char* argv0 = NULL;

static ctrl_subsystem_t* registered_subsystems[] = {
    &screen,
    &wallpaper,
    NULL
};

static int print_usage( void ) {
    int i;

    printf( "%s subsystem command\n\n", argv0 );
    printf( "Available subsystems:\n" );

    for ( i = 0; registered_subsystems[ i ] != NULL; i++ ) {
        printf( "  %s\n", registered_subsystems[ i ]->name );
    }

    return 0;
}

static int print_subsystem_usage( ctrl_subsystem_t* subsystem ) {
    printf( "todo: subsystem usage!\n" );

    return 0;
}

static ctrl_subsystem_t* subsystem_find( char* name ) {
    int i;

    for ( i = 0; registered_subsystems[ i ] != NULL; i++ ) {
        if ( strcmp( registered_subsystems[ i ]->name, name ) == 0 ) {
            return registered_subsystems[ i];
        }
    }

    return NULL;
}

static ctrl_command_t* command_find( ctrl_subsystem_t* subsystem, char* name ) {
    int i;

    for ( i = 0; subsystem->commands[ i ].name != NULL; i++ ) {
        if ( strcmp( subsystem->commands[ i ].name, name ) == 0 ) {
            return &subsystem->commands[ i ];
        }
    }

    return NULL;
}

int main( int argc, char** argv ) {
    ctrl_subsystem_t* subsystem;
    ctrl_command_t* command;

    argv0 = argv[ 0 ];

    if ( application_init( APP_NONE ) != 0 ) {
        fprintf( stderr, "%s: failed to init application.\n", argv0 );
        return EXIT_FAILURE;
    }

    if ( argc < 3 ) {
        print_usage();
        return EXIT_FAILURE;
    }

    subsystem = subsystem_find( argv[ 1 ] );

    if ( subsystem == NULL ) {
        print_usage();
        return EXIT_FAILURE;
    }

    command = command_find( subsystem, argv[ 2 ] );

    if ( command == NULL ) {
        print_subsystem_usage( subsystem );
        return EXIT_FAILURE;
    }

    command->handler( argc - 3, &argv[ 3 ] );

    return EXIT_SUCCESS;
}
