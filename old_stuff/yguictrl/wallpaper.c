/* yaOSp GUI control application
 *
 * Copyright (c) 2010 Kasper Mikkelsen, Zoltan Kovacs
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
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <ygui/desktop.h>
#include <ygui/protocol.h>

#include "yguictrl.h"

int send_set_wallpaper_msg_to_desktop( char* data ) {
    ipc_port_id port;

    get_named_ipc_port( "desktop", &port );
    send_ipc_message( port, MSG_DESKTOP_CHANGE_WALLPAPER, data, strlen( data ) + 1 );

    return 0;
}

static int change_wallpaper( int argc, char** argv ) {
    struct stat st;

    if ( argc != 1 ) {
        fprintf( stderr, "New wallpeper file parameter is missing.\n" );
        return -1;
    }

    if ( stat( argv[0], &st ) != 0 ) {
        fprintf( stderr, "Failed to set new wallpaper: %s.\n", strerror(errno) );
        return -1;
    }

    if ( !S_ISREG( st.st_mode ) ) {
        fprintf( stderr, "%s is not a regular file.\n", argv[0] );
        return -1;
    }

    printf( "Set new wallpaper to: %s.\n", argv[0] );
    send_set_wallpaper_msg_to_desktop( argv[0] );

    return 0;
}

static ctrl_command_t wallpaper_commands[] = {
    { "set", "changes the current wallpaper", change_wallpaper },
    { NULL, NULL }
};

ctrl_subsystem_t wallpaper = {
    .name = "wallpaper",
    .commands = wallpaper_commands
};