/* Device management
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

#ifndef _DEVICES_H_
#define _DEVICES_H_

#include <types.h>
#include <lib/hashtable.h>

typedef struct bus_driver {
    hashitem_t hash;

    char* name;
    void* bus;
} bus_driver_t;

typedef struct device_geometry {
    uint32_t bytes_per_sector;
    uint64_t sector_count;
} device_geometry_t;

int register_bus_driver( const char* name, void* bus );
int unregister_bus_driver( const char* name );

void* get_bus_driver( const char* name );

int init_devices( void );

#endif /* _DEVICES_H_ */
