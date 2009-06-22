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
#include <mm/region.h>
#include <vfs/devfs.h>
#include <gui/graphicsdriver.h>
#include <lib/string.h>

#include <arch/io.h>
#include <arch/cpu.h>
#include <arch/bios.h>

#include "vesa.h"

static uint32_t mode_count;
static uint16_t* mode_table;

static uint32_t screen_mode_count;
static screen_mode_t* screen_mode_table;
static vesa_mode_t* vesa_mode_table;

static void* framebuffer_address = NULL;
static region_id framebuffer_region = -1;

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

    kprintf( "Detected VESA %d.%d compatible graphics card.\n", vesa_info->version >> 8, vesa_info->version & 0xF );

    mode_ptr = ( uint16_t* )( ( ( vesa_info->video_mode_ptr & 0xFFFF0000 ) >> 12 ) |
                              ( vesa_info->video_mode_ptr & 0xFFFF ) );

    tmp = mode_ptr;

    /* Count the usable VESA video modes */

    for ( ; *tmp != 0xFFFF; tmp++ )  {

    }

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

static uint32_t vesa_get_screen_mode_count( void ) {
    return screen_mode_count;
}

static int vesa_get_screen_mode_info( uint32_t index, screen_mode_t* screen_mode ) {
    if ( __unlikely( index >= screen_mode_count ) ) {
        return -EINVAL;
    }

    memcpy( screen_mode, &screen_mode_table[ index ], sizeof( screen_mode_t ) );

    return 0;
}

static int vesa_set_screen_mode( screen_mode_t* screen_mode ) {
    int error;
    vesa_mode_t* vesa_mode;

    vesa_mode = ( vesa_mode_t* )screen_mode->private;

    error = vesa_set_mode( vesa_mode->mode_id );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    framebuffer_region = create_region(
        "vesa_framebuffer",
        screen_mode->width * screen_mode->height * colorspace_to_bpp( screen_mode->color_space ),
        REGION_READ | REGION_WRITE | REGION_KERNEL,
        ALLOC_NONE,
        &framebuffer_address
    );

    if ( framebuffer_region < 0 ) {
        return framebuffer_region;
    }

    error = remap_region( framebuffer_region, ( ptr_t )vesa_mode->phys_base_ptr );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int vesa_get_framebuffer_info( void** address ) {
    *address = framebuffer_address;

    return 0;
}

static graphics_driver_t vesa_driver = {
    .name = "VESA",
    .get_screen_mode_count = vesa_get_screen_mode_count,
    .get_screen_mode_info = vesa_get_screen_mode_info,
    .set_screen_mode = vesa_set_screen_mode,
    .get_framebuffer_info = vesa_get_framebuffer_info,
    .fill_rect = NULL,
    .blit_bitmap = NULL
};

static int init_vesa_graphics_device( void ) {
    int error;
    uint32_t i;
    screen_mode_t* screen_mode;
    vesa_mode_t* vesa_mode;
    vesa_mode_info_t mode_info;

    /* Create the table for the screen modes */

    screen_mode_table = ( screen_mode_t* )kmalloc( sizeof( screen_mode_t ) * mode_count );

    if ( __unlikely( screen_mode_table == NULL ) ) {
        error = -ENOMEM;
        goto error1;
    }

    vesa_mode_table = ( vesa_mode_t* )kmalloc( sizeof( vesa_mode_t ) * mode_count );

    if ( __unlikely( vesa_mode_table == NULL ) ) {
        error = -ENOMEM;
        goto error2;
    }

    /* Run through the VESA modes */

    screen_mode_count = 0;
    screen_mode = screen_mode_table;
    vesa_mode = vesa_mode_table;

    for ( i = 0; i < mode_count; i++ ) {
        error = vesa_get_mode_information( mode_table[ i ], &mode_info );

        if ( __unlikely( error < 0 ) ) {
            continue;
        }

        if ( ( mode_info.phys_base_ptr == 0 ) ||
             ( mode_info.num_planes != 1 ) ||
             ( ( mode_info.bits_per_pixel != 16 ) &&
               ( mode_info.bits_per_pixel != 24 ) &&
               ( mode_info.bits_per_pixel != 32 ) ) ) {
            continue;
        }

        screen_mode->width = mode_info.width;
        screen_mode->height = mode_info.height;
        screen_mode->color_space = bpp_to_colorspace( mode_info.bits_per_pixel );
        screen_mode->private = ( void* )vesa_mode;

        vesa_mode->mode_id = mode_table[ i ];
        vesa_mode->phys_base_ptr = ( void* )mode_info.phys_base_ptr;

        screen_mode++;
        vesa_mode++;
        screen_mode_count++;
    }

    kprintf( "Found %u usable VESA modes.\n", screen_mode_count );

    /* Register the graphics driver */

    error = register_graphics_driver( &vesa_driver );

    if ( error < 0 ) {
        goto error3;
    }

    kprintf( "Registered VESA graphics driver.\n" );

    return 0;

 error3:
    kfree( vesa_mode_table );

 error2:
    kfree( screen_mode_table );

 error1:
    return error;
}

int init_module( void ) {
    int error;

    error = detect_vesa();

    if ( error < 0 ) {
        return error;
    }

    error = init_vesa_graphics_device();

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
