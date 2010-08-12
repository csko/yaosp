/* Kernel part of the VMware graphics driver
 *
 * Copyright (c) 2010 Zoltan Kovacs
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

#include <errno.h>
#include <console.h>
#include <pci.h>
#include <devices.h>

int init_module(void) {
    int i;
    int dev_count;
    pci_bus_t* pci_bus;

    pci_bus = get_bus_driver( "PCI" );

    if (pci_bus == NULL) {
        kprintf(WARNING, "vmware: PCI bus not found!\n");
        return -1;
    }

    dev_count = pci_bus->get_device_count();

    for (i = 0; i < dev_count; i++) {
        pci_device_t* pci_device;

        pci_device = pci_bus->get_device(i);
    }

    return 0;
}

int destroy_module(void) {
    return 0;
}
