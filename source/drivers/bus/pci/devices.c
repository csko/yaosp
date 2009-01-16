/* PCI device handling under device filesystem
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
#include <macros.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include "pci.h"

typedef struct pci_cookie {
    char* buffer;
    size_t size;
    size_t position;
} pci_cookie_t;

static pci_cookie_t* create_pci_cookie( size_t size ) {
    pci_cookie_t* cookie;

    cookie = ( pci_cookie_t* )kmalloc( sizeof( pci_cookie_t ) + size );

    if ( cookie == NULL ) {
        return NULL;
    }

    cookie->buffer = ( char* )( cookie + 1 );
    cookie->size = size;
    cookie->position = 0;

    return cookie;
}

static int vendor_open( void* node, uint32_t flags, void** _cookie ) {
    char tmp[ 8 ];
    size_t length;
    pci_device_t* device;
    pci_cookie_t* cookie;

    device = ( pci_device_t* )node;

    length = snprintf( tmp, sizeof( tmp ), "0x%X", device->vendor_id );

    cookie = create_pci_cookie( length );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    memcpy( cookie->buffer, tmp, length );

    *_cookie = ( void* )cookie;

    return 0;
}

static int vendor_close( void* node, void* cookie ) {
    kfree( cookie );

    return 0;
}

static int vendor_read( void* node, void* _cookie, void* buffer, off_t position, size_t size ) {
    size_t to_read;
    pci_cookie_t* cookie;

    cookie = ( pci_cookie_t* )_cookie;
    to_read = MIN( size, cookie->size - cookie->position );

    if ( to_read == 0 ) {
        return 0;
    }

    memcpy( buffer, cookie->buffer + cookie->position, to_read );
    cookie->position += to_read;

    return to_read;
}

static device_calls_t vendor_calls = {
    .open = vendor_open,
    .close = vendor_close,
    .ioctl = NULL,
    .read = vendor_read,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static int device_open( void* node, uint32_t flags, void** _cookie ) {
    char tmp[ 8 ];
    size_t length;
    pci_device_t* device;
    pci_cookie_t* cookie;

    device = ( pci_device_t* )node;

    length = snprintf( tmp, sizeof( tmp ), "0x%X", device->device_id );

    cookie = create_pci_cookie( length );

    if ( cookie == NULL ) {
        return -ENOMEM;
    }

    memcpy( cookie->buffer, tmp, length );

    *_cookie = ( void* )cookie;

    return 0;
}

static int device_close( void* node, void* cookie ) {
    kfree( cookie );
    return 0;
}

/* NOTE: We can use the same function for the read operation
         here because they use the same buffer for the cookie */

static device_calls_t device_calls = {
    .open = device_open,
    .close = device_close,
    .ioctl = NULL,
    .read = vendor_read,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

int create_device_node_for_pci_device( pci_device_t* pci_device ) {
    int error;
    char path[ 64 ];

    /* Create the vendor node */

    snprintf(
        path,
        sizeof( path ),
        "pci/%d:%d:%d/vendor",
        pci_device->bus,
        pci_device->dev,
        pci_device->func
    );

    error = create_device_node( path, &vendor_calls, ( void* )pci_device );

    if ( error < 0 ) {
        return error;
    }

    /* Create the device node */

    snprintf(
        path,
        sizeof( path ),
        "pci/%d:%d:%d/device",
        pci_device->bus,
        pci_device->dev,
        pci_device->func
    );

    error = create_device_node( path, &device_calls, ( void* )pci_device );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
