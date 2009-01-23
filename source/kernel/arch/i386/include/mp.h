/* Intel MultiProcessor specification definitions
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

#ifndef _ARCH_MP_H_
#define _ARCH_MP_H_

#include <types.h>

typedef struct mp_floating_pointer {
    uint8_t signature[ 4 ];
    uint32_t physical_address;
    uint8_t length;
    uint8_t spec_rev;
    uint8_t checksum;
    uint8_t mp_feature[ 5 ];
} __attribute__(( packed )) mp_floating_pointer_t;

typedef struct mp_configuration_table {
    uint8_t signature[ 4 ];
    uint16_t base_table_length;
    uint8_t spec_rev;
    uint8_t checksum;
    uint8_t oem_string[ 8 ];
    uint8_t product_id[ 12 ];
    uint32_t oem_table;
    uint16_t oem_table_size;
    uint16_t entry_count;
    uint32_t local_apic_address;
    uint16_t extended_table_length;
    uint8_t extended_table_checksum;
    uint8_t reserved;
} __attribute__(( packed )) mp_configuration_table_t;

typedef struct mp_processor {
    uint8_t type;
    uint8_t local_apic_id;
    uint8_t local_apic_version;
    unsigned enabled : 1;
    unsigned bootstrap : 1;
    unsigned reserved1 : 6;
    uint32_t signature;
    uint32_t feature_flags;
    uint32_t reserved2;
    uint32_t reserved3;
} __attribute__(( packed )) mp_processor_t;

enum {
    MP_PROCESSOR = 0,
    MP_BUS = 1,
    MP_IOAPIC = 2,
    MP_IO_INT_ASSIGN = 3,
    MP_LOCAL_INT_ASSIGN = 4
};

int init_mp( void );

#endif // _ARCH_MP_H_
