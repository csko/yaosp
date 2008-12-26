/* PCI bus definitions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _PCI_H_
#define _PCI_H_

#include <types.h>

#define PCI_VENDOR_ID   0x00 /* (2 byte) vendor id */
#define PCI_DEVICE_ID   0x02 /* (2 byte) device id */
#define PCI_COMMAND     0x04 /* (2 byte) command */
#define PCI_STATUS      0x06 /* (2 byte) status */
#define PCI_REVISION    0x08 /* (1 byte) revision id */
#define PCI_CLASS_API   0x09 /* (1 byte) specific register interface type */
#define PCI_CLASS_SUB   0x0A /* (1 byte) specific device function */
#define PCI_CLASS_BASE  0x0B /* (1 byte) device type (display vs network, etc) */
#define PCI_LINE_SIZE   0x0C /* (1 byte) cache line size in 32 bit words */
#define PCI_LATENCY     0x0D /* (1 byte) latency timer */
#define PCI_HEADER_TYPE 0x0E /* (1 byte) header type */
#define PCI_BIST        0x0F /* (1 byte) built-in self-test */

#define PCI_HEADER_BRIDGE 0x01 /* PCI bridge */
#define PCI_MULTIFUNCTION 0x80 /* multifunction device flag */

#define PCI_BUS_PRIMARY   0x18 /* primary side of the PCI bridge */
#define PCI_BUS_SECONDARY 0x19 /* secondary side fo the PCI bridge */

#define MAX_PCI_DEVICES 256

typedef struct pci_device {
    int bus;
    int dev;
    int func;

    uint16_t vendor_id;
    uint16_t device_id;
} pci_device_t;

typedef struct pci_bus {
    int ( *get_device_count )( void );
    pci_device_t* ( *get_device )( int index );
} pci_bus_t;

#endif // _PCI_H_
