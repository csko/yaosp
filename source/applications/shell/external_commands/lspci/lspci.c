/* PCI device lister
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

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "deviceid.h"

char* argv0 = NULL;

static int read_pci_entry_node( char* dir, char* node, char* buffer, size_t size ) {
    int fd;
    int data;
    char path[ 64 ];

    snprintf( path, sizeof( path ), "/device/pci/%s/%s", dir, node );

    fd = open( path, O_RDONLY );

    if ( fd < 0 ) {
        return fd;
    }

    data = read( fd, buffer, size - 1 );

    close( fd );

    if ( data < 0 ) {
        return data;
    }

    buffer[ data ] = 0;

    return 0;
}

static void list_pci_entry( char* dir ) {
    int error;
    long vendor_id;
    long device_id;
    char vendor[ 8 ];
    char device[ 8 ];
    int index;
    char* vendor_name;
    char* device_name;

    error = read_pci_entry_node( dir, "vendor", vendor, sizeof( vendor ) );

    if ( error < 0 ) {
        return;
    }

    error = read_pci_entry_node( dir, "device", device, sizeof( device ) );

    if ( error < 0 ) {
        return;
    }

    vendor_id = strtol( vendor, NULL, 16 );
    device_id = strtol( device, NULL, 16 );

    index = get_vendor( vendor_id );

    if ( index == -1 ) {
        vendor_name = "Unknown vendor";
        device_name = "Unknown device";
    } else {
        vendor_name = ( char* )pci_device_id_list[ index ].name;
        device_name = ( char* )get_device_name( device_id, index );

        if ( device_name == NULL ) {
            device_name = "Unknown device";
        }
    }

    printf( "%s %s:%s %s %s\n", dir, vendor, device, vendor_name, device_name );
}

int main( int argc, char** argv ) {
    DIR* dir;
    struct dirent* entry;

    argv0 = argv[ 0 ];

    dir = opendir( "/device/pci" );

    if ( dir == NULL ) {
        printf( "%s: Failed to open /device/pci!\n", argv0 );
        return EXIT_FAILURE;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        list_pci_entry( entry->d_name );
    }

    closedir( dir );

    return EXIT_SUCCESS;
}
