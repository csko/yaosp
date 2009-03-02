/* PCI Device ID list
 *
 * Copyright (c) 2009 Kornel Csernai, Zoltan Kovacs
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

/*
 * This list is based on Craig Hart's pcidevs.txt:
 * http://members.datafast.net.au/dft0802/downloads/pcidevs.txt
 */

#ifndef _DEVICEID_H_
#define _DEVICEID_H_

#include <sys/types.h>

typedef struct vendor_info {
    uint16_t vendor_id;
    const char* name;
    uint32_t device_start;
    uint32_t device_count;
} vendor_info_t;

typedef struct device_info {
    uint16_t device_id;
    const char* name;
} device_info_t;

vendor_info_t* get_vendor_info( uint16_t id );
device_info_t* get_device_info( uint16_t id, uint32_t start, uint32_t count );

#endif  // _DEVICEID_H_
