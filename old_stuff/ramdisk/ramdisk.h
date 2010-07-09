/* RAM disk driver
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

#ifndef _RAMDISK_H_
#define _RAMDISK_H_

#include <types.h>
#include <lib/hashtable.h>

typedef struct ramdisk_node {
    hashitem_t hash;

    int id;
    uint64_t size;
    void* data;
} ramdisk_node_t;

typedef struct ramdisk_create_info {
    uint64_t size;
    int load_from_file;
    char image_file[ 256 ];
    char node_name[ 32 ];
} ramdisk_create_info_t;

ramdisk_node_t* create_ramdisk_node( ramdisk_create_info_t* info );

int init_ramdisk_control_device( void );

#endif // _RAMDISK_H_
