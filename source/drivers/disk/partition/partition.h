/* Partition table parser
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#ifndef _PARTITION_H_
#define _PARTITION_H_

#include <types.h>

typedef struct partition_type {
    const char* name;
    int ( *scan )( const char* device );
} partition_type_t;

extern partition_type_t msdos_partition;

int create_device_partition( int fd, const char* device, int index, off_t offset, uint64_t size );

#endif /* _PARTITION_H_ */
