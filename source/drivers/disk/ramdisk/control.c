/* RAM disk driver
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
#include <errno.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "ramdisk.h"

static int ramdisk_do_create( ramdisk_create_info_t* info ) {
    ramdisk_node_t* node;

    node = create_ramdisk_node( info );

    if ( node == NULL ) {
        return -1;
    }

    snprintf( info->node_name, 32, "ram%d", node->id );

    return 0;
}

static int ramdisk_control_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    switch ( command ) {
        case IOCTL_RAMDISK_CREATE :
            return ramdisk_do_create( ( ramdisk_create_info_t* )args );

        default :
            return -ENOSYS;
    }
}

static device_calls_t ramdisk_control_calls = {
    .open = NULL,
    .close = NULL,
    .ioctl = ramdisk_control_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int init_ramdisk_control_device( void ) {
    int error;

    error = create_device_node( "control/ramdisk", &ramdisk_control_calls, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
