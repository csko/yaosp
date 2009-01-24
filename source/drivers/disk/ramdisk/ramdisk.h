/* RAM disk driver
 *
 * Copyright (c) 2009 Kornel Csernai
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

typedef struct ramdisk_node {
    int id;
    size_t size;
    void* data;
} ramdisk_node_t;

static int create_ramdisk_node(ramdisk_node_t* ramdisk);
static int destroy_ramdisk_node(ramdisk_node_t* ramdisk);

#endif // _RAMDISK_H_
