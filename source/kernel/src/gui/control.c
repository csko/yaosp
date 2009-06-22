/* GUI controller interface
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

#include <config.h>

#ifdef ENABLE_GUI

#include <macros.h>
#include <ioctl.h>
#include <errno.h>
#include <console.h>
#include <vfs/devfs.h>
#include <gui/gui.h>

static int gui_ctrl_open( void* node, uint32_t flags, void** cookie ) {
    return 0;
}

static int gui_ctrl_close( void* node, void* cookie ) {
    return 0;
}

static int gui_ctrl_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;

    switch ( command ) {
        case IOCTL_GUI_START :
            error = gui_start();
            break;

        case IOCTL_GUI_STOP :
            error = 0;
            break;

        default :
            kprintf( "gui_ctrl_ioctl(): Unknown command: %x\n", command );
            error = -ENOSYS;
            break;
    }

    return error;
}

static device_calls_t gui_ctrl_calls = {
    .open = gui_ctrl_open,
    .close = gui_ctrl_close,
    .ioctl = gui_ctrl_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

__init int init_gui_control( void ) {
    int error;

    error = create_device_node( "control/gui", &gui_ctrl_calls, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

#endif /* ENABLE_GUI */
