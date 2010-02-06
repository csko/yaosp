#include <stdio.h>
#include <stdlib.h>
#include <ygui/desktop.h>
#include <ygui/protocol.h>

#include "yguictrl.h"

int send_set_wallpaper_msg_to_desktop( char* data ) {
    ipc_port_id port;

    get_named_ipc_port( "desktop", &port );
    send_ipc_message( port, MSG_DESKTOP_CHANGE_WALLPAPER, data, strlen(data) + 1 );

    return 0;
}

static int change_wallpaper( int argc, char** argv) {
    printf( "Set new wallpaper to: %s.\n", argv[0] );
    send_set_wallpaper_msg_to_desktop( argv[0] );
    return 0;
}

static ctrl_command_t wallpaper_commands[] = {
    { "set", change_wallpaper },
    { NULL, NULL }
};

ctrl_subsystem_t wallpaper = {
    .name = "wallpaper",
    .commands = wallpaper_commands
};
