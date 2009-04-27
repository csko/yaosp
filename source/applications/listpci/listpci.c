/* PCI device lister
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

    /* Strip the newline at the end */
    buffer[ data - 1 ] = 0;

    return 0;
}

static void list_pci_entry( char* dir ) {
    int error;
    int vendor_id;
    int device_id;
    char vendor[ 16 ];
    char device[ 16 ];
/*
    TODO: Determine revision, subsystem vendor and subsystem device
          Print them when in debug mode (-v etc.)

    char revision[ 16 ];
    char subsystem_vendor[ 16 ];
    char subsystem_device[ 16 ];
*/

    const char* vendor_name;
    const char* device_name;
    vendor_info_t* vendor_info;
    device_info_t* device_info;

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

    vendor_info = get_vendor_info( vendor_id );

    if ( vendor_info == NULL ) {
        vendor_name = "Unknown vendor";
        device_name = "Unknown device";
    } else {
        vendor_name = vendor_info->name;

        device_info = get_device_info( device_id, vendor_info->device_start, vendor_info->device_count );

        if ( device_info == NULL ) {
            device_name = "Unknown device";
        } else {
            device_name = device_info->name;
        }
    }

    printf( "%s %04x:%04x %s %s\n", dir, vendor_id, device_id, vendor_name, device_name );
}

int main( int argc, char** argv ) {
    DIR* dir;
    struct dirent* entry;

    argv0 = argv[ 0 ];

    dir = opendir( "/device/pci" );

    if ( dir == NULL ) {
        fprintf( stderr, "%s: Failed to open /device/pci!\n", argv0 );
        return EXIT_FAILURE;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        list_pci_entry( entry->d_name );
    }

    closedir( dir );

    return EXIT_SUCCESS;
}
