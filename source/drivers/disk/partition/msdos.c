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
#include <console.h>
#include <vfs/devfs.h>
#include <vfs/vfs.h>

#include "partition.h"

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

static int msdos_scan_partitions( const char* device ) {
    int i;
    int fd;
    int error;
    char path[ 128 ];
    char buffer[ 512 ];
    partition_t* partition;
    partition_table_t* table;

    snprintf( path, sizeof( path ), "/device/storage/%s", device );

    fd = open( path, O_RDWR );

    if ( fd < 0 ) {
        error = fd;
        goto error1;
    }

    if ( pread( fd, buffer, 512, 0 ) != 512 ) {
        error = -EIO;
        goto error2;
    }

    table = ( partition_table_t* )buffer;

    if ( table->signature != 0xAA55 ) {
        error = -EINVAL;
        goto error2;
    }

    for ( i = 0; i < 4; i++ ) {
        int new_fd;

        partition = &table->partitions[ i ];

        if ( partition->type == 0 ) {
            continue;
        }

        new_fd = dup( fd );

        if ( new_fd < 0 ) {
            kprintf( ERROR, "msdos_scan_partitions(): Failed to duplicate file descriptor! (error=%d)\n", new_fd );
            continue;
        }

        error = create_device_partition(
            new_fd,
            device,
            i,
            ( off_t )partition->start_lba * 512,
            ( uint64_t )partition->num_sectorss * 512
        );

        if ( error < 0 ) {
            kprintf( ERROR, "msdos_scan_partitions(): Failed to create partition device! (error=%d)\n", error );
            close( new_fd );
        }
    }

    error = 0;

error2:
    close( fd );

error1:
    return error;
}

partition_type_t msdos_partition = {
    .name = "MS/DOS",
    .scan = msdos_scan_partitions
};
