/* Terminal driver
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

#include <ioctl.h>
#include <vfs/devfs.h>

#include "terminal.h"

static int term_ctrl_open( void* node, uint32_t flags, void** cookie ) {
    return 0;
}

static int term_ctrl_close( void* node, void* cookie ) {
    return 0;
}

static int term_ctrl_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    switch ( command ) {
        case IOCTL_TERM_SET_ACTIVE : {
            int active;

            active = *( ( int* )args );

            mutex_lock( terminal_lock );
            terminal_switch_to( active );
            mutex_unlock( terminal_lock );

            break;
        }
    }

    return 0;
}

static device_calls_t term_ctrl_calls = {
    .open = term_ctrl_open,
    .close = term_ctrl_close,
    .ioctl = term_ctrl_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int init_terminal_ctrl_device( void ) {
    int error;

    error = create_device_node( "control/terminal", &term_ctrl_calls, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
