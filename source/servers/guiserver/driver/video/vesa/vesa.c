/* VESA graphics driver
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

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <yaosp/region.h>

#include "vesa.h"

static int vesa_device = -1;

static uint32_t mode_count = 0;
static vesa_screen_mode_t* mode_table = NULL;
static vesa_screen_mode_t* current_mode = NULL;

static region_id fb_region = -1;
static void* fb_address = NULL;

static int vesa_detect( void ) {
    int error;
    uint32_t i;
    uint16_t mode_id_table[ 128 ];
    vesa_mode_info_t* mode_info;
    vesa_cmd_modelist_t modelist_cmd;
    vesa_cmd_modeinfo_t modeinfo_cmd;

    /* Open the VESA device */

    vesa_device = open( "/device/video/vesa", O_RDONLY );

    if ( vesa_device < 0 ) {
        goto error1;
    }

    /* Get the list of supported VESA video modes */

    modelist_cmd.max_count = 128;
    modelist_cmd.mode_list = mode_id_table;

    error = ioctl( vesa_device, IOCTL_VESA_GET_MODE_LIST, ( void* )&modelist_cmd );

    if ( error < 0 ) {
        goto error2;
    }

    /* In the case of an empty list we're done */

    if ( modelist_cmd.current_count == 0 ) {
        return 0;
    }

    mode_table = ( vesa_screen_mode_t* )malloc( sizeof( vesa_screen_mode_t ) * modelist_cmd.current_count );

    if ( mode_table == NULL ) {
        goto error2;
    }

    mode_count = 0;
    mode_info = &modeinfo_cmd.mode_info;

    for ( i = 0; i < modelist_cmd.current_count; i++ ) {
        modeinfo_cmd.mode_number = mode_id_table[ i ];

        error = ioctl( vesa_device, IOCTL_VESA_GET_MODE_INFO, ( void* )&modeinfo_cmd );

        if ( error < 0 ) {
            continue;
        }

        if ( ( mode_info->phys_base_ptr == 0 ) ||
             ( mode_info->num_planes != 1 ) ||
             ( ( mode_info->bits_per_pixel != 16 ) && ( mode_info->bits_per_pixel != 24 ) && ( mode_info->bits_per_pixel != 32 ) ) ) {
            continue;
        }

        mode_table[ mode_count ].screen_mode.width = mode_info->width;
        mode_table[ mode_count ].screen_mode.height = mode_info->height;
        mode_table[ mode_count ].screen_mode.color_space = bpp_to_colorspace( mode_info->bits_per_pixel );

        mode_table[ mode_count ].vesa_mode_id = mode_id_table[ i ];
        mode_table[ mode_count ].phys_base_ptr = ( void* )mode_info->phys_base_ptr;

        mode_count++;
    }

    mode_table = ( vesa_screen_mode_t* )realloc( mode_table, sizeof( vesa_screen_mode_t ) * mode_count );

    for ( i = 0; i < mode_count; i++ ) {
        mode_table[ i ].screen_mode.private = &mode_table[ i ];
    }

    printf( "Found %d usable VESA video modes.\n", mode_count );

    return 0;

error2:
    close( vesa_device );
    vesa_device = -1;

error1:
    return -1;
}

static uint32_t vesa_get_mode_count( void ) {
    return mode_count;
}

static int vesa_get_mode_info( uint32_t index, screen_mode_t* screen_mode ) {
    if ( index >= mode_count ) {
        return -EINVAL;
    }

    memcpy( screen_mode, ( void* )&mode_table[ index ].screen_mode, sizeof( screen_mode_t ) );

    return 0;
}

static int vesa_set_mode( screen_mode_t* screen_mode ) {
    int error;
    vesa_screen_mode_t* vesa_mode;
    vesa_cmd_setmode_t setmode_cmd;

    vesa_mode = ( vesa_screen_mode_t* )screen_mode->private;
    setmode_cmd.mode_number = vesa_mode->vesa_mode_id | ( 1 << 14 );

    error = ioctl( vesa_device, IOCTL_VESA_SET_MODE, ( void* )&setmode_cmd );

    if ( error < 0 ) {
        return error;
    }

    current_mode = vesa_mode;

    fb_region = memory_region_create(
        "vesa_fb",
        current_mode->screen_mode.width * current_mode->screen_mode.height * colorspace_to_bpp( current_mode->screen_mode.color_space ),
        REGION_READ | REGION_WRITE,
        &fb_address
    );

    if ( fb_region < 0 ) {
        return fb_region;
    }

    error = memory_region_remap_pages( fb_region, vesa_mode->phys_base_ptr );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int vesa_get_framebuffer_info( void** address ) {
    *address = fb_address;

    return 0;
}

graphics_driver_t vesa_graphics_driver = {
    .name = "VESA",
    .detect = vesa_detect,
    .get_screen_mode_count = vesa_get_mode_count,
    .get_screen_mode_info = vesa_get_mode_info,
    .set_screen_mode = vesa_set_mode,
    .get_framebuffer_info = vesa_get_framebuffer_info,
    .fill_rect = generic_fill_rect,
    .draw_text = generic_draw_text,
    .blit_bitmap = generic_blit_bitmap
};
