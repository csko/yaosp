/* Input driver
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

#include <errno.h>
#include <ioctl.h>
#include <input.h>
#include <vfs/devfs.h>

#include "input.h"

static int input_ctrl_open( void* node, uint32_t flags, void** cookie ) {
    return 0;
}

static int input_ctrl_close( void* node, void* cookie ) {
    return 0;
}

static int input_ctrl_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;

    switch ( command ) {
        case IOCTL_INPUT_CREATE_DEVICE : {
            input_device_t* device;
            input_cmd_create_node_t* cmd;

            device = create_input_device( 0 );

            if ( device != NULL ) {
                error = insert_input_device( device );

                if ( error >= 0 ) {
                    cmd = ( input_cmd_create_node_t* )args;

                    cmd->node_number = device->node_number;
                } else {
                    destroy_input_device( device );
                }
            } else {
                error = -ENOMEM;
            }

            break;
        }

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static device_calls_t input_ctrl_calls = {
    .open = input_ctrl_open,
    .close = input_ctrl_close,
    .ioctl = input_ctrl_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int init_input_controller( void ) {
    int error;

    error = create_device_node( "control/input", &input_ctrl_calls, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
