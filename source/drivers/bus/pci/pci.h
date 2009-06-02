/* PCI bus definitions
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

#ifndef _PCI_H_
#define _PCI_H_

#include <types.h>

#define PCI_VENDOR_ID      0x00 /* (2 byte) vendor id */
#define PCI_DEVICE_ID      0x02 /* (2 byte) device id */
#define PCI_COMMAND        0x04 /* (2 byte) command */
#define PCI_STATUS         0x06 /* (2 byte) status */
#define PCI_REVISION_ID    0x08 /* (1 byte) revision id */
#define PCI_CLASS_API      0x09 /* (1 byte) specific register interface type */
#define PCI_CLASS_SUB      0x0A /* (1 byte) specific device function */
#define PCI_CLASS_BASE     0x0B /* (1 byte) device type (display vs network, etc) */
#define PCI_LINE_SIZE      0x0C /* (1 byte) cache line size in 32 bit words */
#define PCI_LATENCY        0x0D /* (1 byte) latency timer */
#define PCI_HEADER_TYPE    0x0E /* (1 byte) header type */
#define PCI_BIST           0x0F /* (1 byte) built-in self-test */
#define PCI_BASE_REGISTERS 0x10 /* base registers (size varies) */
#define PCI_SUBSYS_VEND_ID 0x2C /* (2 byte) subsystem vendor id */
#define PCI_SUBSYS_DEV_ID  0x2E /* (2 byte) subsystem id */
#define PCI_INTERRUPT_LINE 0x3C /* (1 byte) interrupt line */

#define PCI_HEADER_BRIDGE 0x01 /* PCI bridge */
#define PCI_MULTIFUNCTION 0x80 /* multifunction device flag */

#define PCI_BUS_PRIMARY   0x18 /* primary side of the PCI bridge */
#define PCI_BUS_SECONDARY 0x19 /* secondary side fo the PCI bridge */

/* Possible values for PCI command */

#define PCI_COMMAND_IO           0x001 /* 1/0 i/o space en/disabled */
#define PCI_COMMAND_MEMORY       0x002 /* 1/0 memory space en/disabled */
#define PCI_COMMAND_MASTER       0x004 /* 1/0 pci master en/disabled */
#define PCI_COMMAND_SPECIAL      0x008 /* 1/0 pci special cycles en/disabled */
#define PCI_COMMAND_MWI          0x010 /* 1/0 memory write & invalidate en/disabled */
#define PCI_COMMAND_VGA_SNOOP    0x020 /* 1/0 vga pallette snoop en/disabled */
#define PCI_COMMAND_PARITY       0x040 /* 1/0 parity check en/disabled */
#define PCI_COMMAND_ADDRESS_STEP 0x080 /* 1/0 address stepping en/disabled */
#define PCI_COMMAND_SERR         0x100 /* 1/0 SERR# en/disabled */
#define PCI_COMMAND_FASTBACK     0x200 /* 1/0 fast back-to-back en/disabled */
#define PCI_COMMAND_INT_DISABLE  0x400 /* 1/0 interrupt generation dis/enabled */

/* Possible values for class base */

#define PCI_EARLY                 0x00 /* built before class codes defined */
#define PCI_MASS_STORAGE          0x01 /* mass storage_controller */
#define PCI_NETWORK               0x02 /* network controller */
#define PCI_DISPLAY               0x03 /* display controller */
#define PCI_MULTIMEDIA            0x04 /* multimedia device */
#define PCI_MEMORY                0x05 /* memory controller */
#define PCI_BRIDGE                0x06 /* bridge controller */
#define PCI_SIMPLE_COMMUNICATIONS 0x07 /* simple communications controller */
#define PCI_BASE_PERIPHERAL       0x08 /* base system peripherals */
#define PCI_INPUT                 0x09 /* input devices */
#define PCI_DOCKING_STATION       0x0A /* docking stations */
#define PCI_PROCESSOR             0x0B /* processors */
#define PCI_SERIAL_BUS            0x0C /* serial_bus_controller */
#define PCI_UNDEFINED             0xFF /* not in any defined class */

/* Possible values for mass storage subclass */

#define PCI_SCSI               0x00 /* SCSI controller */
#define PCI_IDE                0x01 /* IDE controller */
#define PCI_FLOPPY             0x02 /* floppy disk controller */
#define PCI_IPI                0x03 /* IPI bus controller */
#define PCI_RAID               0x03 /* RAID controller */
#define PCI_MASS_STORAGE_OTHER 0x80 /* other mass storage controller */

#define PCI_ADDRESS_IO_MASK        0xFFFFFFFC
#define PCI_ADDRESS_MEMORY_32_MASK 0xFFFFFFF0
#define PCI_ADDRESS_MEMORY_64_MASK 0xFFFFFFFFFFFFFFF0ULL

#define MAX_PCI_DEVICES 256

typedef struct pci_device {
    int bus;
    int dev;
    int func;

    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision_id;
    uint8_t class_api;
    uint8_t class_sub;
    uint8_t class_base;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_device_id;

    int interrupt_line;
    uint32_t base[ 6 ];
} pci_device_t;

typedef struct pci_bus {
    int ( *get_device_count )( void );
    pci_device_t* ( *get_device )( int index );
    int ( *enable_device )( pci_device_t* device, uint32_t flags );
    int ( *read_config )( pci_device_t* device, int offset, int size, uint32_t* data );
    int ( *write_config )( pci_device_t* device, int offset, int size, uint32_t data );
} pci_bus_t;

int create_device_node_for_pci_device( pci_device_t* pci_device );

#endif /* _PCI_H_ */
