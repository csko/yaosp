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
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <vfs/devfs.h>
#include <lib/string.h>

typedef struct partition {
    uint8_t bootable;
    uint8_t start_head;
    uint8_t start_sector;
    uint8_t start_cylinder;
    uint8_t type;
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;
    uint32_t start_lba;
    uint32_t num_sectorss;
} __attribute__(( packed )) partition_t;

typedef struct partition_table {
    uint8_t bootcode[ 446 ];
    partition_t partitions[ 4 ];
    uint16_t signature;
} __attribute__(( packed )) partition_table_t;

typedef struct partition_node {
    int fd;
    off_t offset;
    uint64_t size;
} partition_node_t;

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
    .ioctl = NULL,
    .read = partition_read,
    .write = partition_write,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static int create_device_partition( int fd, const char* device, int index, off_t offset, uint64_t size ) {
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

    return 0;
}

static int check_device_partitions( const char* device ) {
    int i;
    int fd;
    int error;
    char path[ 128 ];
    char buffer[ 512 ];
    int partitions_created;
    partition_t* partition;
    partition_table_t* table;

    snprintf( path, sizeof( path ), "/device/storage/%s", device );

    fd = open( path, O_RDWR );

    if ( fd < 0 ) {
        return fd;
    }

    if ( pread( fd, buffer, 512, 0 ) != 512 ) {
        close( fd );
        return -EIO;
    }

    table = ( partition_table_t* )buffer;

    if ( table->signature != 0xAA55 ) {
        kprintf( "Invalid partition signature on %s\n", device );
        close( fd );
        return -EIO;
    }

    partitions_created;

    for ( i = 0; i < 4; i++ ) {
        partition = &table->partitions[ i ];

        if ( partition->type == 0 ) {
            continue;
        }

        error = create_device_partition(
            fd,
            device,
            i,
            ( off_t )partition->start_lba * 512,
            ( uint64_t )partition->num_sectorss * 512
        );

        if ( error == 0 ) {
            partitions_created++;
        }
    }

    if ( partitions_created == 0 ) {
        close( fd );
    }

    return 0;
}

int init_module( void ) {
    int dir;
    dirent_t entry;
    int error;

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
            kprintf( "Failed to detect partitions on: /device/storage/%s\n", entry.name );
        }
    }

    close( dir );

    return 0;
}

int destroy_module( void ) {
    return 0;
}

MODULE_OPTIONAL_DEPENDENCIES( "pata" );
