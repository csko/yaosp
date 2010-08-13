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
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/string.h>

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    const char* name;
} pci_entry_t;

enum VMwareIoctls {
    VMWARE_GET_IO_BASE = 0x00100000
};

static int vmware_card_count = 0;

static pci_entry_t vmware_pci_table[] = {
    { 0x15AD, 0x0405, "VMware SVGA2" }
};

static int vmware_ioctl(void* node, void* cookie, uint32_t command, void* args, bool from_kernel) {
    int error;

    switch (command) {
        case VMWARE_GET_IO_BASE : {
            int* arg = (int*)args;
            pci_device_t* dev = (pci_device_t*)node;

            *arg = dev->base[0] & PCI_ADDRESS_IO_MASK;
            error = 0;

            break;
        }

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static device_calls_t vmware_calls = {
    .open = NULL,
    .close = NULL,
    .ioctl = vmware_ioctl,
    .read = NULL,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static int vmware_create_node(pci_bus_t* bus, pci_device_t* dev) {
    char path[64];
    void* data;

    data = kmalloc(sizeof(pci_device_t));

    if (data == NULL) {
        return -ENOMEM;
    }

    memcpy(data, dev, sizeof(pci_device_t));
    snprintf(path, sizeof(path), "video/vmware%d", vmware_card_count++);

    uint32_t tmp;
    bus->read_config(dev, PCI_COMMAND, 2, &tmp);
    tmp |= PCI_COMMAND_IO;
    tmp |= PCI_COMMAND_MEMORY;
    bus->write_config(dev, PCI_COMMAND, 2, tmp);

    return create_device_node(path, &vmware_calls, data);
}

int init_module(void) {
    int i;
    int dev_count;
    pci_bus_t* pci_bus;

    pci_bus = get_bus_driver("PCI");

    if (pci_bus == NULL) {
        kprintf(WARNING, "vmware: PCI bus not found!\n");
        return -1;
    }

    dev_count = pci_bus->get_device_count();

    for (i = 0; i < dev_count; i++) {
        int j;
        pci_device_t* pci_device;

        pci_device = pci_bus->get_device(i);

        for (j = 0; j < ARRAY_SIZE(vmware_pci_table); j++) {
            pci_entry_t* vmware_dev = &vmware_pci_table[j];

            if ((pci_device->vendor_id == vmware_dev->vendor_id) &&
                (pci_device->device_id == vmware_dev->device_id)) {
                kprintf(INFO, "Found graphics card: %s.\n", vmware_dev->name);
                vmware_create_node(pci_bus, pci_device);

                break;
            }
        }
    }

    return 0;
}

int destroy_module(void) {
    return 0;
}
