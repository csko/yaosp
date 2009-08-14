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

#include <console.h>
#include <module.h>

#include "pata.h"

int controller_count = 0;
pata_controller_t* controllers = NULL;

int init_module( void ) {
    int error;
    pata_controller_t* controller;

    error = pata_detect_controllers();

    if ( error < 0 ) {
        kprintf( ERROR, "PATA: Failed to detect controllers!\n" );
        return error;
    }

    kprintf( INFO, "PATA: Detected %d controller(s)\n", controller_count );

    controller = controllers;

    while ( controller != NULL ) {
        error = pata_initialize_controller( controller );

        if ( error < 0 ) {
            kprintf(
                ERROR,
                "PATA: Failed to initialize controller at %d:%d:%d\n",
                controller->pci_device.bus,
                controller->pci_device.dev,
                controller->pci_device.func
            );
        }

        controller = controller->next;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}

MODULE_DEPENDENCIES( "pci" );
