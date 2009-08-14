/* Parallel AT Attachment driver
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#include <devices.h>
#include <errno.h>
#include <console.h>
#include <thread.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/io.h>

#include "pata.h"

static uint16_t command_ports[] = { 0x1F0, 0x1F0, 0x170, 0x170 };
static uint16_t control_ports[] = { 0x3F6, 0x3F6, 0x376, 0x376 };

int pata_detect_controllers( void ) {
    int i;
    int device_count;
    pci_bus_t* pci_bus;
    pci_device_t* pci_device;
    pata_controller_t* controller;

    pci_bus = get_bus_driver( "PCI" );

    if ( pci_bus == NULL ) {
        kprintf( WARNING, "PATA: PCI bus not found!\n" );
        return -1;
    }

    device_count = pci_bus->get_device_count();

    for ( i = 0; i < device_count; i++ ) {
        pci_device = pci_bus->get_device( i );

        if ( ( pci_device->class_base == PCI_MASS_STORAGE ) &&
             ( pci_device->class_sub == PCI_IDE ) ) {
            kprintf(
                INFO,
                "PATA: Detected controller at %d:%d:%d\n",
                pci_device->bus,
                pci_device->dev,
                pci_device->func
            );

            controller = ( pata_controller_t* )kmalloc( sizeof( pata_controller_t ) );

            if ( controller == NULL ) {
                return -ENOMEM;
            }

            memset( controller, 0, sizeof( pata_controller_t ) );
            memcpy( &controller->pci_device, pci_device, sizeof( pci_device_t ) );

            controller->next = controllers;
            controllers = controller;

            controller_count++;
        }
    }

    return 0;
}

static int pata_enable_controller( pata_controller_t* controller ) {
    int error;
    pci_bus_t* pci_bus;

    pci_bus = get_bus_driver( "PCI" );

    if ( __unlikely( pci_bus == NULL ) ) {
        kprintf( WARNING, "PATA: PCI bus not found!\n" );

        return -1;
    }

    error = pci_bus->enable_device( &controller->pci_device, PCI_COMMAND_MASTER | PCI_COMMAND_IO );

    if ( __unlikely( error < 0 ) ) {
        kprintf( ERROR, "PATA: Failed to enable controller!\n" );

        return error;
    }

    return 0;
}

static int pata_reset_channel( pata_controller_t* controller, int channel, uint32_t* signatures ) {
    pata_port_t* master;
    pata_port_t* slave;

    master = controller->ports[ channel * controller->ports_per_channel ];
    slave = controller->ports[ channel * controller->ports_per_channel + 1 ];

    master->present = pata_is_port_present( master );
    slave->present = pata_is_port_present( slave );

    /* Select the master */

    pata_port_select( master );

    /* Disable interrupts and start software reset */

    outb(
        PATA_CONTROL_DEFAULT | PATA_CONTROL_INTDISABLE | PATA_CONTROL_SW_RESET,
        master->ctrl_base
    );
    inb( master->ctrl_base );

    thread_sleep( 20 );

    /* Stop software reset */

    outb(
        PATA_CONTROL_DEFAULT | PATA_CONTROL_INTDISABLE,
        master->ctrl_base
    );
    inb( master->ctrl_base );

    thread_sleep( 15000 );

    if ( master->present ) {
        uint8_t count;
        uint8_t lba_low;
        uint8_t lba_mid;
        uint8_t lba_high;
        uint8_t error;

        pata_port_select( master );

        /* Wait until the busy flag is cleared */

        if ( pata_port_wait( master, 0, PATA_STATUS_BUSY, false, 31000000 ) < 0 ) {
            kprintf( WARNING, "PATA: Reset timed out!\n" );
            return -ETIME;
        }

        count = inb( master->cmd_base + PATA_REG_COUNT );
        lba_low = inb( master->cmd_base + PATA_REG_LBA_LOW );
        lba_mid = inb( master->cmd_base + PATA_REG_LBA_MID );
        lba_high = inb( master->cmd_base + PATA_REG_LBA_HIGH );
        error = inb( master->cmd_base + PATA_REG_ERROR );

        if ( ( error != 0x01 ) && ( error != 0x81 ) ) {
            kprintf( WARNING, "PATA: Master failed (error=%x)\n", error );
        }

        if ( error >= 0x80 ) {
            kprintf( WARNING, "PATA: Slave failed as master said! (error=%x)\n", error );
        }

        signatures[ 0 ] =
            count |
            ( uint32_t )lba_low << 8 |
            ( uint32_t )lba_mid << 16 |
            ( uint32_t )lba_high << 24;
    }

    if ( slave->present ) {
        uint8_t count;
        uint8_t lba_low;
        uint8_t lba_mid;
        uint8_t lba_high;
        uint8_t error;

        pata_port_select( slave );

        /* Wait until the busy flag is cleared */

        if ( pata_port_wait( master, 0, PATA_STATUS_BUSY, false, 31000000 ) < 0 ) {
            kprintf( WARNING, "PATA: Reset timed out!\n" );
            return -ETIME;
        }

        count = inb( master->cmd_base + PATA_REG_COUNT );
        lba_low = inb( master->cmd_base + PATA_REG_LBA_LOW );
        lba_mid = inb( master->cmd_base + PATA_REG_LBA_MID );
        lba_high = inb( master->cmd_base + PATA_REG_LBA_HIGH );
        error = inb( master->cmd_base + PATA_REG_ERROR );

        if ( error != 0x01 ) {
            kprintf( WARNING, "PATA: Slave failed (error=%x)\n", error );
        }

        signatures[ 1 ] =
            count |
            ( uint32_t )lba_low << 8 |
            ( uint32_t )lba_mid << 16 |
            ( uint32_t )lba_high << 24;
    }

    return 0;
}

int pata_initialize_controller( pata_controller_t* controller ) {
    int chan;
    int port_num;
    int error;
    pata_port_t* port;

    /* Enable the controller */

    error = pata_enable_controller( controller );

    if ( error < 0 ) {
        return error;
    }

    /* Initialize ports */

    controller->channels = 2;
    controller->ports_per_channel = 2;

    for ( chan = 0; chan < controller->channels; chan++ ) {
        for ( port_num = 0; port_num < controller->ports_per_channel; port_num++ ) {
            port = ( pata_port_t* )kmalloc( sizeof( pata_port_t ) );

            if ( port == NULL ) {
                kprintf( ERROR, "PATA: No memory for port!\n" );
                return -ENOMEM;
            }

            memset( port, 0, sizeof( pata_port_t ) );

            /* First we make all the ports present and later the
               probe will mark it as non-present if required. */

            port->present = true;

            port->channel = chan;
            port->is_slave = ( ( port_num % 2 ) != 0 );

            port->cmd_base = command_ports[ chan * controller->ports_per_channel + port_num ];
            port->ctrl_base = control_ports[ chan * controller->ports_per_channel + port_num ];

            controller->ports[ chan * controller->ports_per_channel + port_num ] = port;
        }
    }

    /* Reset the channels of the controller */

    for ( chan = 0; chan < controller->channels; chan++ ) {
        uint32_t signatures[ 2 ];

        error = pata_reset_channel( controller, chan, signatures );

        if ( error < 0 ) {
            kprintf( WARNING, "PATA: Failed to reset channel %d\n", chan );
            return error;
        }

        for ( port_num = 0; port_num < controller->ports_per_channel; port_num++ ) {
            port = controller->ports[ chan * controller->ports_per_channel + port_num ];

            if ( port->present ) {
                port->is_atapi = ( signatures[ port_num ] == 0xEB140101 );
            } else {
                controller->ports[ chan * controller->ports_per_channel + port_num ] = NULL;
                kfree( port );
            }
        }
    }

    /* Identify and configure present ports */

    for ( chan = 0; chan < controller->channels; chan++ ) {
        for ( port_num = 0; port_num < controller->ports_per_channel; port_num++ ) {
            port = controller->ports[ chan * controller->ports_per_channel + port_num ];

            /* Check if this port is present */

            if ( port == NULL ) {
                continue;
            }

            /* Identify the port. In the case of a failure the port
               will be marked as non-present. */

            error = pata_port_identify( port );

            if ( error < 0 ) {
                controller->ports[ chan * controller->ports_per_channel + port_num ] = NULL;
                kfree( port );
                continue;
            }

            /* Configure the port */

            if ( port->is_atapi ) {
                error = pata_configure_atapi_port( port );
            } else {
                error = pata_configure_ata_port( port );
            }

            if ( error < 0 ) {
                controller->ports[ chan * controller->ports_per_channel + port_num ] = NULL;
                kfree( port );
                continue;
            }
        }
    }

    /* Display port informations and create the device nodes */

    for ( chan = 0; chan < controller->channels; chan++ ) {
        int used;
        lock_id mutex;

        used = 0;

        mutex = mutex_create( "PATA port mutex", MUTEX_NONE );

        if ( mutex < 0 ) {
            return mutex;
        }

        for ( port_num = 0; port_num < controller->ports_per_channel; port_num++ ) {
            port = controller->ports[ chan * controller->ports_per_channel + port_num ];

            if ( port == NULL ) {
                kprintf( INFO, "PATA: Device %d:%d not present\n", chan, port_num );
            } else {
                port->mutex = mutex;
                used++;

                if ( port->is_atapi ) {
                    kprintf( INFO, "PATA: Device %d:%d is ATAPI\n", chan, port_num );
                } else {
                    kprintf( INFO, "PATA: Device %d:%d is ATA\n", chan, port_num );
                }

                kprintf( INFO, "PATA: Model: %s\n", port->model_name );

                if ( port->is_atapi ) {
                    error = pata_create_atapi_device_node( port );
                } else {
                    kprintf( INFO, "PATA: Capacity: %d Mb\n", ( uint32_t )( port->capacity / 1000000 ) );

                    error = pata_create_ata_device_node( port );
                }

                if ( error < 0 ) {
                    return error;
                }
            }
        }

        if ( used == 0 ) {
            mutex_destroy( mutex );
        }
    }

    return 0;
}
