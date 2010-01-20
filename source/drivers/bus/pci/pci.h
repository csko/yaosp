/* PCI bus definitions
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
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

#ifndef _PCI_PCI_H_
#define _PCI_PCI_H_

#include <types.h>
#include <pci.h>

#define MAX_PCI_DEVICES 256

int create_device_node_for_pci_device( pci_device_t* pci_device );

#endif /* _PCI_PCI_H_ */
