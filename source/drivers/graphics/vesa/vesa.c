/* VESA driver
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
#include <console.h>
#include <ioctl.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include <arch/io.h>
#include <arch/cpu.h>
#include <arch/bios.h>

#include "vesa.h"

static uint32_t mode_count;
static uint16_t* mode_table = NULL;

static int detect_vesa( void ) {
    int error;
    bios_regs_t regs;
    vesa_info_t* vesa_info;

    uint16_t* tmp;
    uint16_t* mode_ptr;

    vesa_info = ( vesa_info_t* )0x2000;

    memset( &regs, 0, sizeof( bios_regs_t ) );

    memset( vesa_info, 0, sizeof( vesa_info_t ) );
    memcpy( vesa_info->signature, "VESA", 4 );

    regs.ax = 0x4F00;
    regs.es = 0x0000;
    regs.di = 0x2000;

    error = call_bios_interrupt( 0x10, &regs );

    if ( error < 0 ) {
        return error;
    }

    if ( ( ( regs.ax & 0xFF ) != 0x4F ) ||
         ( ( regs.ax >> 8 ) != 0x00 ) ) {
        return -ENOENT;
    }

    if ( vesa_info->video_mode_ptr == 0x0000 ) {
        return -ENOENT;
    }

    kprintf( INFO, "Detected VESA %d.%d compatible graphics card.\n", vesa_info->version >> 8, vesa_info->version & 0xF );

    mode_ptr = ( uint16_t* )( ( ( vesa_info->video_mode_ptr & 0xFFFF0000 ) >> 12 ) |
                              ( vesa_info->video_mode_ptr & 0xFFFF ) );

    tmp = mode_ptr;

    for ( ; *tmp != 0xFFFF; tmp++ ) ;

    mode_count = ( ( uint32_t )tmp - ( uint32_t )mode_ptr ) / sizeof( uint16_t );

    if ( mode_count == 0 ) {
        return 0;
    }

    mode_table = ( uint16_t* )kmalloc( sizeof( uint16_t ) * mode_count );

    if ( mode_table == NULL ) {
        return -ENOMEM;
    }

    memcpy( mode_table, mode_ptr, sizeof( uint16_t ) * mode_count );

    return 0;
}

static int vesa_get_mode_information( uint16_t mode_number, vesa_mode_info_t* mode_info ) {
    int error;
    bios_regs_t regs;
    vesa_mode_info_t* vesa_mode_info;

    vesa_mode_info = ( vesa_mode_info_t* )0x2000;

    memset( &regs, 0, sizeof( bios_regs_t ) );

    memset( vesa_mode_info, 0, sizeof( vesa_mode_info_t ) );

    regs.ax = 0x4F01;
    regs.cx = mode_number;
    regs.es = 0x0000;
    regs.di = 0x2000;

    error = call_bios_interrupt( 0x10, &regs );

    if ( error < 0 ) {
        return error;
    }

    if ( ( ( regs.ax & 0xFF ) != 0x4F ) ||
         ( ( regs.ax >> 8 ) != 0x00 ) ) {
        return -ENOENT;
    }

    memcpy( mode_info, ( void* )0x2000, sizeof( vesa_mode_info_t ) );

    return 0;
}

static int vesa_set_mode( uint16_t mode_number ) {
    int error;
    bios_regs_t regs;

    memset( &regs, 0, sizeof( bios_regs_t ) );

    regs.ax = 0x4F02;
    regs.bx = mode_number;

    error = call_bios_interrupt( 0x10, &regs );

    if ( error < 0 ) {
        return error;
    }

    if ( ( ( regs.ax & 0xFF ) != 0x4F ) ||
         ( ( regs.ax >> 8 ) != 0x00 ) ) {
        return -ENOENT;
    }

    return 0;
}

static int vesa_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;

    switch ( command ) {
        case IOCTL_VESA_GET_MODE_LIST : {
            uint32_t to_copy;
            vesa_cmd_modelist_t* cmd;

            cmd = ( vesa_cmd_modelist_t* )args;
            to_copy = MIN( mode_count, cmd->max_count );

            if ( to_copy > 0 ) {
                memcpy( cmd->mode_list, mode_table, sizeof( uint16_t ) * to_copy );
            }

            cmd->current_count = to_copy;
            error = 0;

            break;
        }

        case IOCTL_VESA_GET_MODE_INFO : {
            vesa_cmd_modeinfo_t* cmd;

            cmd = ( vesa_cmd_modeinfo_t* )args;

            error = vesa_get_mode_information( cmd->mode_number, &cmd->mode_info );

            break;
        }

        case IOCTL_VESA_SET_MODE : {
            vesa_cmd_setmode_t* cmd;

            cmd = ( vesa_cmd_setmode_t* )args;

            error = vesa_set_mode( cmd->mode_number );

            break;
        }

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static device_calls_t vesa_calls = {
    .open = NULL,
    .close = NULL,
    .ioctl = vesa_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static int create_vesa_device( void ) {
    int error;

    error = create_device_node( "video/vesa", &vesa_calls, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int init_module( void ) {
    int error;

    error = detect_vesa();

    if ( error < 0 ) {
        return error;
    }

    error = create_vesa_device();

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
