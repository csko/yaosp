/* Advanced Configuration and Power Interface
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

#ifndef _I386_ACPI_H_
#define _I386_ACPI_H_

#include <types.h>
#include <macros.h>

#define PMTMR_TICKS_PER_SEC 3579545
#define ACPI_PM_OVRRUN      ( 1 << 24 )

typedef struct acpi_header {
    uint8_t signature[ 4 ];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem_id[ 6 ];
    uint8_t oem_table_id[ 8 ];
    uint32_t oem_revision;
    uint8_t creator_id[ 4 ];
    uint32_t creator_revision;
} __PACKED acpi_header_t;

typedef struct acpi_rsdp {
    uint8_t signature[ 8 ];
    uint8_t checksum;
    uint8_t oem_id[ 6 ];
    uint8_t revision;
    uint32_t rsdt;
} __PACKED acpi_rsdp_t;

typedef struct acpi_fadt {
    acpi_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t pref_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;
} __PACKED acpi_fadt_t;

uint32_t acpi_pmtimer_read( void );

int acpi_init( void );

#endif /* _I386_ACPI_H_ */
