/* Partition table parser
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

#include <types.h>
#include <errno.h>
#include <module.h>
#include <console.h>
#include <ioctl.h>
#include <devices.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "partition.h"

typedef struct partition_node {
    int fd;
    off_t offset;
    uint64_t size;
} partition_node_t;

static partition_type_t* partition_types[] = {
    &msdos_partition,
    NULL
};

static int partition_ioctl( void* _node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;
    partition_node_t* node;

    node = ( partition_node_t* )_node;

    switch ( command ) {
        case IOCTL_DISK_GET_GEOMETRY : {
            device_geometry_t* geometry;

            geometry = ( device_geometry_t* )args;

            geometry->bytes_per_sector = 512;
            geometry->sector_count = node->size / 512;

            error = 0;

            break;
        }

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static int partition_read( void* _node, void* cookie, void* buffer, off_t position, size_t size ) {
    partition_node_t* node;

    node = ( partition_node_t* )_node;

    if ( position > node->size ) {
        return 0;
    }

    if ( position + size > node->size ) {
        size = node->size - position;
    }

    if ( size == 0 ) {
        return 0;
    }

    return pread( node->fd, buffer, size, position + node->offset );
}

static int partition_write( void* _node, void* cookie, const void* buffer, off_t position, size_t size ) {
    partition_node_t* node;

    node = ( partition_node_t* )_node;

    if ( position > node->size ) {
        return 0;
    }

    if ( position + size > node->size ) {
        size = node->size - position;
    }

    if ( size == 0 ) {
        return 0;
    }

    return pwrite( node->fd, buffer, size, position + node->offset );
}

static device_calls_t partition_calls = {
    .open = NULL,
    .close = NULL,
    .ioctl = partition_ioctl,
    .read = partition_read,
    .write = partition_write,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int create_device_partition( int fd, const char* device, int index, off_t offset, uint64_t size ) {
    int error;
    char path[ 128 ];
    partition_node_t* node;

    node = ( partition_node_t* )kmalloc( sizeof( partition_node_t ) );

    if ( node == NULL ) {
        return -ENOMEM;
    }

    snprintf( path, sizeof( path ), "storage/partition/%s%d", device, index );

    node->fd = fd;
    node->offset = offset;
    node->size = size;

    error = create_device_node( path, &partition_calls, ( void* )node );

    if ( error < 0 ) {
        return error;
    }

    kprintf( INFO, "Created device node for partition: /device/%s\n", path );

    return 0;
}

static int check_device_partitions( const char* device ) {
    int i;
    int error;

    for ( i = 0; partition_types[ i ] != NULL; i++ ) {
        error = partition_types[ i ]->scan( device );

        if ( error == 0 ) {
            break;
        }
    }

    return 0;
}

int init_module( void ) {
    int dir;
    dirent_t entry;
    int error;

    error = mkdir( "/device/storage/partition", 0777 );

    if ( error < 0 ) {
        kprintf( ERROR, "Failed to create /device/storage/partition!\n" );
        return error;
    }

    dir = open( "/device/storage", O_RDONLY );

    if ( dir < 0 ) {
        return dir;
    }

    while ( getdents( dir, &entry, sizeof( dirent_t ) ) == 1 ) {
        if ( ( strcmp( entry.name, "." ) == 0 ) ||
             ( strcmp( entry.name, ".." ) == 0 ) ) {
            continue;
        }

        error = check_device_partitions( entry.name );

        if ( error < 0 ) {
            kprintf( ERROR, "Failed to detect partitions on: /device/storage/%s\n", entry.name );
        }
    }

    close( dir );

    return 0;
}

int destroy_module( void ) {
    return 0;
}

MODULE_OPTIONAL_DEPENDENCIES( "pata" );
