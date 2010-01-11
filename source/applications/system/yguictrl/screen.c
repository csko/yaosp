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

#include "yguictrl.h"

static int screen_list_modes( int argc, char** argv ) {
    int i;
    int size;
    array_t* mode_list;

    mode_list = desktop_get_screen_modes();
    size = array_get_size( mode_list );

    if ( size == 0 ) {
        fprintf( stderr, "There are no available screen modes.\n" );
        goto out;
    }

    for ( i = 0; i < size; i++ ) {
        screen_mode_info_t* mode_info;

        mode_info = ( screen_mode_info_t* )array_get_item( mode_list, i );

        printf(
            "%dx%dx%d\n", mode_info->width, mode_info->height,
            colorspace_to_bpp( mode_info->color_space ) * 8
        );
    }

 out:
    desktop_put_screen_modes( mode_list );

    return 0;
}

static int screen_set_mode( int argc, char** argv ) {
    int width;
    int height;
    int depth;
    screen_mode_info_t mode_info;

    char* sep1;
    char* sep2;

    if ( argc != 1 ) {
        fprintf( stderr, "%s: screen setmode mode\n", argv0 );
        return 0;
    }

    sep1 = strchr( argv[ 0 ], 'x' );

    if ( sep1 == NULL ) {
        return 0;
    }

    sep2 = strchr( sep1 + 1, 'x' );

    if ( sep2 == NULL ) {
        return 0;
    }

    width = strtol( argv[ 0 ], NULL, 10 );
    height = strtol( sep1 + 1, NULL, 10 );
    depth = strtol( sep2 + 1, NULL, 10 );

    if ( ( depth != 16 ) &&
         ( depth != 24 ) &&
         ( depth != 32 ) ) {
        fprintf( stderr, "%s: invalid color depth: %d.\n", argv0, depth );
        return 0;
    }

    mode_info.width = width;
    mode_info.height = height;
    mode_info.color_space = bpp_to_colorspace( depth );

    desktop_set_screen_mode( &mode_info );

    return 0;
}

static ctrl_command_t screen_commands[] = {
    { "listmodes", screen_list_modes },
    { "setmode", screen_set_mode },
    { NULL, NULL }
};

ctrl_subsystem_t screen = {
    .name = "screen",
    .commands = screen_commands
};
