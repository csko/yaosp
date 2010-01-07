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

#include <macros.h>
#include <console.h>
#include <errno.h>
#include <mm/region.h>
#include <mm/context.h>
#include <lib/string.h>

#include <arch/acpi.h>
#include <arch/io.h>
#include <arch/hpet.h>

int acpi_pmtmr_found = 0;
uint32_t acpi_pmtmr_ioport;

static uint32_t acpi_rsdp_places[] = {
    /* Bottom 1k of base memory */
    0x0, 0x400,
    /* 64k of BIOS */
    0xE0000, 0x20000
};

static uint8_t ACPI_RSDP_SIGNATURE[] = { 'R', 'S', 'D', ' ', 'P', 'T', 'R', ' ' };
static uint8_t ACPI_RSDT_SIGNATURE[] = { 'R', 'S', 'D', 'T' };
static uint8_t ACPI_FADT_SIGNATURE[] = { 'F', 'A', 'C', 'P' };
static uint8_t ACPI_HPET_SIGNATURE[] = { 'H', 'P', 'E', 'T' };

static inline int acpi_checksum( uint8_t* data, size_t size ) {
    int sum = 0;

    while ( size-- > 0 ) {
        sum += *data++;
    }

    return ( sum & 0xFF );
}

static int acpi_find_rsdp_at( ptr_t address, uint32_t size, uint32_t* rsdt ) {
    int ret;
    uint32_t i;
    uint8_t* data;
    memory_region_t* region;

    region = do_create_memory_region( &kernel_memory_context, "acpi_rsdp",
                                      PAGE_ALIGN( size ), REGION_KERNEL | REGION_READ );

    do_memory_region_remap_pages( region, address );

    data = ( uint8_t* )region->address;

    for ( i = 0; i < size; i += 16, data += 16 ) {
        acpi_rsdp_t* rsdp;

        rsdp = ( acpi_rsdp_t* )data;

        if ( ( memcmp( rsdp->signature, ACPI_RSDP_SIGNATURE, 8 ) != 0 ) ||
             ( acpi_checksum( ( uint8_t* )rsdp, sizeof( acpi_rsdp_t ) ) != 0 ) ) {
            continue;
        }

        *rsdt = rsdp->rsdt;
        ret = 0;

        goto found;
    }

    ret = -1;

 found:
    do_memory_region_put( region );

    return ret;
}

static int acpi_find_rsdp( uint32_t* rsdt ) {
    int i;

    for ( i = 0; i < ARRAY_SIZE( acpi_rsdp_places ); i += 2 ) {
        if ( acpi_find_rsdp_at( acpi_rsdp_places[ i ], acpi_rsdp_places[ i + 1 ], rsdt ) == 0 ) {
            return 0;
        }
    }

    return -1;
}

static int acpi_parse_fadt( acpi_fadt_t* fadt ) {
    acpi_pmtmr_found = 1;
    acpi_pmtmr_ioport = fadt->pm_tmr_blk;

    return 0;
}

static int acpi_parse_hpet( acpi_hpet_t* hpet ) {
    if ( hpet->address.space_id != ACPI_SPACE_MEMORY ) {
        kprintf( WARNING, "HPET timers must be located in memory.\n" );
        return 0;
    }

    hpet_address = hpet->address.address;
    hpet_present = ( hpet_address != 0 );

    return 0;
}

static int acpi_parse_table_at( uint32_t table_address ) {
    uint32_t length;
    acpi_header_t* header;
    memory_region_t* region;

    region = do_create_memory_region( &kernel_memory_context, "acpi_table",
                                      PAGE_ALIGN( sizeof( acpi_header_t ) + ( table_address & ~PAGE_MASK ) ),
                                      REGION_KERNEL | REGION_READ );

    do_memory_region_remap_pages( region, table_address & PAGE_MASK );

    header = ( acpi_header_t* )( region->address + ( table_address & ~PAGE_MASK ) );
    length = header->length;

    do_memory_region_put( region );

    region = do_create_memory_region( &kernel_memory_context, "acpi_table",
                                      PAGE_ALIGN( length + ( table_address & ~PAGE_MASK ) ),
                                      REGION_KERNEL | REGION_READ );

    do_memory_region_remap_pages( region, table_address & PAGE_MASK );

    header = ( acpi_header_t* )( region->address + ( table_address & ~PAGE_MASK ) );

    if ( acpi_checksum( ( uint8_t* )header, length ) != 0 ) {
        kprintf( WARNING, "ACPI: invalid table checksum.\n" );
        goto out;
    }

    if ( memcmp( header->signature, ACPI_FADT_SIGNATURE, 4 ) == 0 ) {
        acpi_parse_fadt( ( acpi_fadt_t* )header );
    } else if ( memcmp( header->signature, ACPI_HPET_SIGNATURE, 4 ) == 0 ) {
        acpi_parse_hpet( ( acpi_hpet_t* )header );
    }

 out:
    do_memory_region_put( region );

    return 0;
}

static int acpi_parse_rsdt( uint32_t rsdt_address ) {
    uint32_t i;
    uint32_t length;
    uint32_t* ptr;
    uint32_t entry_count;
    acpi_header_t* header;
    memory_region_t* region;

    region = do_create_memory_region( &kernel_memory_context, "acpi_rsdt",
                                      PAGE_ALIGN( sizeof( acpi_header_t ) + ( rsdt_address & ~PAGE_MASK ) ),
                                      REGION_KERNEL | REGION_READ );

    do_memory_region_remap_pages( region, rsdt_address & PAGE_MASK );

    header = ( acpi_header_t* )( region->address + ( rsdt_address & ~PAGE_MASK ) );

    if ( memcmp( header->signature, ACPI_RSDT_SIGNATURE, 4 ) != 0 ) {
        kprintf( WARNING, "ACPI: invalid RSDT signature.\n" );
        goto out;
    }

    length = header->length;

    do_memory_region_put( region );

    region = do_create_memory_region( &kernel_memory_context, "acpi_rsdt",
                                      PAGE_ALIGN( length + ( rsdt_address & ~PAGE_MASK ) ),
                                      REGION_KERNEL | REGION_READ );

    do_memory_region_remap_pages( region, rsdt_address & PAGE_MASK );

    memory_context_insert_region( &kernel_memory_context, region );

    header = ( acpi_header_t* )( region->address + ( rsdt_address & ~PAGE_MASK ) );

    if ( acpi_checksum( ( uint8_t* )header, length ) != 0 ) {
        kprintf( WARNING, "ACPI: invalid RSDT checksum.\n" );
        goto out;
    }

    entry_count = ( length - sizeof( acpi_header_t ) ) / 4;

    ptr = ( uint32_t* )( header + 1 );

    for ( i = 0; i < entry_count; i++, ptr++ ) {
        acpi_parse_table_at( *ptr );
    }

 out:
    do_memory_region_put( region );

    return 0;
}

uint32_t acpi_pmtimer_read( void ) {
    if ( !acpi_pmtmr_found ) {
        return 0;
    }

    return inl( acpi_pmtmr_ioport ) & ( ( 1 << 24 ) - 1 );
}

int acpi_init( void ) {
    uint32_t rsdt_address;

    if ( acpi_find_rsdp( &rsdt_address ) != 0 ) {
        return -ENOENT;
    }

    acpi_parse_rsdt( rsdt_address );

    return 0;
}
