/* PCI bus handling
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#include <types.h>
#include <errno.h>
#include <console.h>
#include <macros.h>
#include <devices.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/io.h>
#include <arch/spinlock.h>
#include <arch/interrupt.h>

#include "pci.h"

typedef bool pci_access_check_t( void );
typedef int pci_access_read_t( int bus, int dev, int func, int offset, int size, uint32_t* data );
typedef int pci_access_write_t( int bus, int dev, int func, int offset, int size, uint32_t data );

typedef struct pci_access {
    const char* name;
    int devs_per_bus;

    pci_access_check_t* check;
    pci_access_read_t* read;
    pci_access_write_t* write;
} pci_access_t;

static pci_access_t* pci_access;
static spinlock_t pci_lock = INIT_SPINLOCK( "PCI" );

static int pci_device_count = 0;
static pci_device_t* pci_devices[ MAX_PCI_DEVICES ];

static int pci_scan_bus( int bus );

/* PCI access method 1 */

static bool pci_check_1( void ) {
    bool ints;
    bool works = false;
    uint32_t tmp;

    /* Disable interrupts */

    ints = disable_interrupts();

    /* Check if PCI access method 1 is supported */

    outb( 0x01, 0x0CFB );
    tmp = inb( 0x0CF8 );

    outl( 0x80000000, 0x0CF8 );

    if ( inl( 0x0CF8 ) == 0x80000000 ) {
        works = true;
    }

    outl( tmp, 0x0CF8 );

    /* Enable interrupts (if we disabled them) */

    if ( ints ) {
        enable_interrupts();
    }

    return works;
}

static int pci_read_1( int bus, int dev, int func, int offset, int size, uint32_t* data ) {
    int error = 0;

    if ( ( bus > 255 ) ||
         ( dev > 31 ) ||
         ( func > 7 ) ) {
        return -EINVAL;
    }

    spinlock_disable( &pci_lock );

    outl(
        0x80000000 |
        ( bus << 16 ) |
        ( dev << 11 ) |
        ( func << 8 ) |
        ( offset & ~3 ),
        0x0CF8
    );

    switch ( size ) {
        case 1 :
            *data = inb( 0x0CFC + ( offset & 3 ) );
            break;

        case 2 :
            *data = inw( 0x0CFC + ( offset & 2 ) );
            break;

        case 4 :
            *data = inl( 0x0CFC );
            break;

        default :
            error = -EINVAL;
            break;
    }

    spinunlock_enable( &pci_lock );

    return error;
}

static int pci_write_1( int bus, int dev, int func, int offset, int size, uint32_t data ) {
    int error = 0;

    if ( ( bus > 255 ) ||
         ( dev > 31 ) ||
         ( func > 7 ) ) {
        return -EINVAL;
    }

    spinlock_disable( &pci_lock );

    outl(
        0x80000000 |
        ( bus << 16 ) |
        ( dev << 11 ) |
        ( func << 8 ) |
        ( offset & ~3 ),
        0x0CF8
    );

    switch ( size ) {
        case 1 :
            outb( ( uint8_t )data, 0x0CFC + ( offset & 3 ) );
            break;

        case 2 :
            outw( ( uint16_t )data, 0x0CFC + ( offset & 2 ) );
            break;

        case 4 :
            outl( data, 0x0CFC );
            break;

        default :
            error = -EINVAL;
            break;
    }

    spinunlock_enable( &pci_lock );

    return error;
}

static pci_access_t pci_access_method_1 = {
    .name = "access method 1",
    .devs_per_bus = 32,
    .check = pci_check_1,
    .read = pci_read_1,
    .write = pci_write_1
};

/* PCI access method 2 */

static bool pci_check_2( void ) {
    bool ints;
    bool works = false;

    /* Disable interrupts */

    ints = disable_interrupts();

    /* Check if PCI access method 2 is supported */

    outb( 0x00, 0x0CFB );
    outb( 0x00, 0x0CF8 );
    outb( 0x00, 0x0CFA );

    if ( ( inb( 0x0CF8 ) == 0 ) &&
         ( inb( 0x0CFA ) == 0 ) ) {
        works = true;
    }

    /* Enable interrupts */

    if ( ints ) {
        enable_interrupts();
    }

    return works;
}

static int pci_read_2( int bus, int dev, int func, int offset, int size, uint32_t* data ) {
    int error = 0;

    if ( ( bus > 255 ) ||
         ( dev > 15 ) ||
         ( func > 7 ) ) {
        return -EINVAL;
    }

    spinlock_disable( &pci_lock );

    outb( 0xF0 | ( func << 1 ), 0x0CF8 );
    outb( bus, 0x0CFA );

    switch ( size ) {
        case 1 :
            *data = inb( 0xC000 | ( dev << 8 ) | offset );
            break;

        case 2 :
            *data = inw( 0xC000 | ( dev << 8 ) | offset );
            break;

        case 4 :
            *data = inl( 0xC000 | ( dev << 8 ) | offset );
            break;

        default :
            error = -EINVAL;
            break;
    }

    spinunlock_enable( &pci_lock );

    return error;
}

static int pci_write_2( int bus, int dev, int func, int offset, int size, uint32_t data ) {
    int error = 0;

    if ( ( bus > 255 ) ||
         ( dev > 15 ) ||
         ( func > 7 ) ) {
        return -EINVAL;
    }

    spinlock_disable( &pci_lock );

    outb( 0xF0 | ( func << 1 ), 0x0CF8 );
    outb( bus, 0x0CFA );

    switch ( size ) {
        case 1 :
            outb( ( uint8_t )data, ( 0xC000 | ( dev << 8 ) | offset ) );
            break;

        case 2 :
            outw( ( uint16_t )data, ( 0xC000 | ( dev << 8 ) | offset ) );
            break;

        case 4 :
            outl( data, ( 0xC000 | ( dev << 8 ) | offset ) );
            break;

        default :
            error = -EINVAL;
            break;
    }

    spinunlock_enable( &pci_lock );

    return error;
}

static pci_access_t pci_access_method_2 = {
    .name = "access method 2",
    .devs_per_bus = 16,
    .check = pci_check_2,
    .read = pci_read_2,
    .write = pci_write_2
};

/* Possible PCI access methods */

static pci_access_t* pci_access_methods[] = {
    &pci_access_method_1,
    &pci_access_method_2
};

static bool pci_detect( void ) {
    size_t i;
    pci_access_t* current;

    pci_access = NULL;

    for ( i = 0; i < ARRAY_SIZE( pci_access_methods ); i++ ) {
        current = pci_access_methods[ i ];

        if ( current->check() ) {
            pci_access = current;
            break;
        }
    }

    return ( pci_access != NULL );
}

static int pci_scan_device( int bus, int dev, int func ) {
    int error;
    uint32_t vendor_id;
    uint32_t header_type;

    /* First check if this is a real device */

    error = pci_access->read( bus, dev, func, PCI_VENDOR_ID, 2, &vendor_id );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    if ( ( vendor_id == 0xFFFF ) || ( vendor_id == 0x0000 ) ) {
        return -EINVAL;
    }

    error = pci_access->read( bus, dev, func, PCI_HEADER_TYPE, 1, &header_type );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    /* If this device is a PCI-PCI bridge then scan the new
       bus, otherwise handle as a normal device */

    if ( header_type & PCI_HEADER_BRIDGE ) {
        uint32_t primary;
        uint32_t secondary;

        if ( ( pci_access->read( bus, dev, func, PCI_BUS_PRIMARY, 2, &primary ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_BUS_SECONDARY, 2, &secondary ) < 0 ) ) {
            return -1;
        }

        if ( __unlikely( ( int )primary != bus ) ) {
            return -1;
        }

        if ( __likely( bus != ( int )secondary ) ) {
            pci_scan_bus( ( int )secondary );
        }
    } else {
        int i;
        uint32_t device_id;
        uint32_t revision_id;
        uint32_t class_api;
        uint32_t class_sub;
        uint32_t class_base;
        uint32_t subsystem_vendor_id;
        uint32_t subsystem_device_id;
        uint32_t interrupt_line;
        pci_device_t* device;

        device = ( pci_device_t* )kmalloc( sizeof( pci_device_t ) );

        if ( __unlikely( device == NULL ) ) {
            return -ENOMEM;
        }

        device->bus = bus;
        device->dev = dev;
        device->func = func;

        device->vendor_id = vendor_id;

        if ( ( pci_access->read( bus, dev, func, PCI_DEVICE_ID, 2, &device_id ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_REVISION_ID, 1, &revision_id ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_CLASS_API, 1, &class_api ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_CLASS_SUB, 1, &class_sub ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_CLASS_BASE, 1, &class_base ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_SUBSYS_VEND_ID, 2, &subsystem_vendor_id ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_SUBSYS_DEV_ID, 2, &subsystem_device_id ) < 0 ) ||
             ( pci_access->read( bus, dev, func, PCI_INTERRUPT_LINE, 1, &interrupt_line ) < 0 ) ) {
            return -1;
        }

        for ( i = 0; i < 6; i++ ) {
            error = pci_access->read( bus, dev, func, PCI_BASE_REGISTERS + ( i * 4 ), 4, &device->base[ i ] );

            if ( error < 0 ) {
                return error;
            }
        }

        device->device_id = device_id;
        device->revision_id = revision_id;
        device->class_api = class_api;
        device->class_sub = class_sub;
        device->class_base = class_base;
        device->subsystem_vendor_id = subsystem_vendor_id;
        device->subsystem_device_id = subsystem_device_id;
        device->interrupt_line = interrupt_line;

        if ( device->interrupt_line >= 16 ) {
            device->interrupt_line = 0;
        }

        if ( __likely( pci_device_count < MAX_PCI_DEVICES ) ) {
            kprintf(
                INFO,
                "PCI: %d:%d:%d 0x%04x:0x%04x:0x%x 0x%04x:0x%04x\n",
                bus, dev, func, vendor_id, device_id, revision_id,
                subsystem_vendor_id, subsystem_device_id
            );

            pci_devices[ pci_device_count++ ] = device;

            create_device_node_for_pci_device( device );
        } else {
            kprintf( WARNING, "PCI: Too many devices!\n" );
        }
    }

    return 0;
}

static int pci_bus_get_device_count( void ) {
    return pci_device_count;
}

static pci_device_t* pci_bus_get_device( int index ) {
    if ( __unlikely( ( index < 0 ) || ( index >= pci_device_count ) ) ) {
        return NULL;
    }

    return pci_devices[ index ];
}

static int pci_bus_enable_device( pci_device_t* device, uint32_t flags ) {
    int error;
    uint32_t tmp;

    error = pci_access->read(
        device->bus,
        device->dev,
        device->func,
        PCI_COMMAND,
        2,
        &tmp
    );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    if ( ( tmp & flags ) != flags ) {
        error = pci_access->write(
            device->bus,
            device->dev,
            device->func,
            PCI_COMMAND,
            2,
            tmp | flags
        );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }

        error = pci_access->read(
            device->bus,
            device->dev,
            device->func,
            PCI_COMMAND,
            2,
            &tmp
        );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }

        if ( ( tmp & flags ) != flags ) {
            kprintf( ERROR, "PCI: Failed to enable device at %d:%d:%d!\n", device->bus, device->dev, device->func );

            return -1;
        }
    }

    return 0;
}

static int pci_bus_read_config( pci_device_t* device, int offset, int size, uint32_t* data ) {
    return pci_access->read(
        device->bus,
        device->dev,
        device->func,
        offset,
        size,
        data
    );
}

static int pci_bus_write_config( pci_device_t* device, int offset, int size, uint32_t data ) {
    return pci_access->write(
        device->bus,
        device->dev,
        device->func,
        offset,
        size,
        data
    );
}

static pci_bus_t pci_bus = {
    .get_device_count = pci_bus_get_device_count,
    .get_device = pci_bus_get_device,
    .enable_device = pci_bus_enable_device,
    .read_config = pci_bus_read_config,
    .write_config = pci_bus_write_config
};

static int pci_scan_bus( int bus ) {
    int dev;
    int func;

    int error;
    uint32_t header_type;

    kprintf( INFO, "PCI: Scanning bus: %d\n", bus );

    for ( dev = 0; dev < pci_access->devs_per_bus; dev++ ) {
        /* Is this a multifunctional device? */

        error = pci_access->read( bus, dev, 0, PCI_HEADER_TYPE, 1, &header_type );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }

        if ( header_type & PCI_MULTIFUNCTION ) {
            for ( func = 0; func < 8; func++ ) {
                pci_scan_device( bus, dev, func );
            }
        } else {
            pci_scan_device( bus, dev, 0 );
        }
    }

    return 0;
}

int init_module( void ) {
    int error;

    memset( pci_devices, 0, sizeof( pci_device_t* ) * MAX_PCI_DEVICES );

    /* Detect PCI */

    if ( !pci_detect() ) {
        kprintf( INFO, "PCI: Bus not detected\n" );
        return -EINVAL;
    }

    kprintf( INFO, "PCI: Using %s\n", pci_access->name );

    /* Scan the first PCI bus */

    error = pci_scan_bus( 0 );

    if ( error < 0 ) {
        kprintf( ERROR, "PCI: Failed to scan first bus! (error=%d)\n", error );
        return error;
    }

    kprintf( INFO, "PCI: Detected %d devices.\n", pci_device_count );

    /* Register the PCI bus driver */

    error = register_bus_driver( "PCI", ( void* )&pci_bus );

    if ( error < 0 ) {
        kprintf( ERROR, "PCI: Failed to register the bus! (error=%d)\n", error );
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    int i;

    pci_access = NULL;

    /* Unregister the PCI bus */

    unregister_bus_driver( "PCI" );

    /* Free the allocated device structures */

    for ( i = 0; i < pci_device_count; i++ ) {
        kfree( pci_devices[ i ] );
        pci_devices[ i ] = NULL;
    }

    pci_device_count = 0;

    return 0;
}
