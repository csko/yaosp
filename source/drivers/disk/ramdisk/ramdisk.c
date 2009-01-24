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

#include <mm/pages.h>
#include <console.h>
#include <errno.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "ramdisk.h"

/* TODO: get these from boot parameters */
#define RAMDISK_SIZE 0x800000
#define RAMDISK_NUM 2

static ramdisk_node_t ramdisks[RAMDISK_NUM];

int init_module( void ) {
    int error;
    int i;
    ramdisk_node_t* ramdisk;

    for(i = 0; i < RAMDISK_NUM; i++){
        ramdisk = &(ramdisks[i]);
        ramdisk->size = RAMDISK_SIZE;
        ramdisk->data = alloc_pages(RAMDISK_SIZE / PAGE_SIZE);
        if(ramdisk->data == NULL){
            /* TODO: clean up already allocated memory */
            return -ENOMEM;
        }
        ramdisk->id = i;

        error = create_ramdisk_node(ramdisk);
        if(error < 0){
            return error;
        }
    }

    return 0;
}

int destroy_module( void ) {
    int error;
    int i;

    for(i = 0; i < RAMDISK_NUM; i++){
        error = destroy_ramdisk_node(&(ramdisks[i]));
        if(error < 0){
            return error;
        }
        free_pages(ramdisks[i].data, RAMDISK_SIZE / PAGE_SIZE);
    }

    return 0;
}

static int ramdisk_read(void* node, void* cookie, void* buffer, off_t position, size_t size){
    ramdisk_node_t* ramdisk;

    ramdisk = (ramdisk_node_t*) node;

    if(position < 0 || position >= ramdisk->size){
        return -EINVAL;
    }

    if(position + size > ramdisk->size){
        size = ramdisk->size - position;
    }

    if(size == 0){
        return 0;
    }

    memcpy(buffer, ramdisk->data + position, size);

    return size;
}

static int ramdisk_write(void* node, void* cookie, const void* buffer, off_t position, size_t size){
    ramdisk_node_t* ramdisk;

    ramdisk = (ramdisk_node_t*) node;

    if(position < 0 || position >= ramdisk->size){
        return -EINVAL;
    }

    if(position + size > ramdisk->size){
        size = ramdisk->size - position;
    }

    if(size == 0){
        return 0;
    }

    memcpy(ramdisk->data + position, buffer, size);

    return size;
}

static device_calls_t ramdisk_calls = {
    .open = NULL,
    .close = NULL,
    .ioctl = NULL,
    .read = ramdisk_read,
    .write = ramdisk_write,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static int create_ramdisk_node(ramdisk_node_t* ramdisk){
    int error;
    char name[32];

    snprintf(name, sizeof(name), "disk/ram%d", ramdisk->id);

    kprintf("RAMDisk: Creating device node: /device/%s\n", name);

    error = create_device_node(name, &ramdisk_calls, (void*) ramdisk);

    if(error < 0){
        return error;
    }

    return 0;
}

static int destroy_ramdisk_node(ramdisk_node_t* ramdisk){
/*    int error; */
    char name[32];

    snprintf(name, sizeof(name), "disk/ram%d", ramdisk->id);

    kprintf("RAMDisk: Destroying device node: /device/%s\n", name);
/*
    error = destroy_device_node(name);

    if(error < 0){
        return error;
    }
*/
    return 0;
}
