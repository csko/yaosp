/* Handling of Intel MultiProcessor tables
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

#include <config.h>
#include <macros.h>
#include <console.h>
#include <errno.h>
#include <smp.h>
#include <kernel.h>
#include <lib/string.h>

#include <arch/mp.h>
#include <arch/apic.h>
#include <arch/cpu.h>

/* Memory places where to look for the MP floating pointer */

static uint32_t mp_fp_places[] = {
    /* Extended BIOS data area */
    0x40E0, 0x400,
    /* Last kilobyte of system base memory */
    639 * 0x400, 0x400,
    /* BIOS ROM */
    0xF0000, 0x10000
};

/* The signature of the MP floating pointer structure */

static uint8_t mp_fp_signature[ 4 ] = { '_', 'M', 'P', '_' };

/* The signature of the MP configuration table */

static uint8_t mp_ct_signature[ 4 ] = { 'P', 'C', 'M', 'P' };

static inline uint8_t mp_checksum( uint8_t* data, int length ) {
    uint8_t checksum = 0;

    while ( length-- ) {
        checksum += *data++;
    }

    return checksum;
}

#ifdef ENABLE_SMP
__init static uint32_t mp_handle_cfg_table_entry( ptr_t address ) {
    uint8_t* data;
    uint8_t type;

    data = ( uint8_t* )address;
    type = *data;

    switch ( type ) {
        case MP_PROCESSOR : {
            mp_processor_t* processor;

            if ( __unlikely( processor_count >= MAX_CPU_COUNT ) ) {
                return 20;
            }

            processor = ( mp_processor_t* )data;

            if ( !processor->enabled ) {
                return 20;
            }

            /* Map the local APIC ID of the processor to our processor table */

            apic_to_logical_cpu_id[ processor->local_apic_id ] = processor_count;

            /* Make the processor present and save its local APIC ID */

            processor_table[ processor_count ].present = true;
            arch_processor_table[ processor_count ].apic_id = processor->local_apic_id;

            /* We have one more processor ;) */

            processor_count++;

            return 20;
        }

        case MP_BUS :
            return 8;

        case MP_IOAPIC :
            return 8;

        case MP_IO_INT_ASSIGN :
            return 8;

        case MP_LOCAL_INT_ASSIGN :
            return 8;

        default :
            kprintf( WARNING, "mp_handle_cfg_table_entry(): Unknown entry: %d\n", type & 0xFF );
            return 1;
    }
}
#endif /* ENABLE_SMP */

__init static int mp_handle_cfg_table( void* address ) {
    mp_configuration_table_t* ct;

    ct = ( mp_configuration_table_t* )address;

    /* Validate the MP Configuration table */

    if ( memcmp( ct->signature, mp_ct_signature, sizeof( mp_ct_signature ) ) != 0 ) {
        return -EINVAL;
    }

    if ( mp_checksum( ( uint8_t* )ct, ct->base_table_length ) != 0 ) {
        return -EINVAL;
    }

    /* Save the local APIC base address */

    local_apic_base = ct->local_apic_address;

#ifdef ENABLE_SMP
    int i;
    ptr_t entry_addr;

    /* Parse the configuration table entries */

    entry_addr = ( ptr_t )ct + sizeof( mp_configuration_table_t );

    for ( i = 0; i < ct->entry_count; i++ ) {
        entry_addr += mp_handle_cfg_table_entry( entry_addr );
    }
#endif /* ENABLE_SMP */

    return 0;
}

__init static void mp_handle_default_cfg( void ) {
    /* Initialize the default configuration according to the
       Intel MultiProcessor specification, chapter #5 */

    local_apic_base = 0xFEE00000;

#ifdef ENABLE_SMP
    int i;

    processor_count = 2;

    for ( i = 0; i < 2; i++ ) {
        apic_to_logical_cpu_id[ i ] = i;
        arch_processor_table[ i ].apic_id = i;
    }
#endif /* ENABLE_SMP */
}

__init static bool mp_find_floating_pointer( void* addr, uint32_t size, mp_floating_pointer_t** _fp ) {
    uint32_t offset;
    uint32_t real_size;
    memory_region_t* region;
    mp_floating_pointer_t* fp;

    return 0; /* TODO: MM */

    real_size = PAGE_ALIGN( size + ( ( ptr_t )addr & ~PAGE_MASK ) );

    region = do_create_memory_region_at(
        &kernel_memory_context,
        "mp fp",
        ( ptr_t )addr & PAGE_MASK,
        real_size,
        REGION_READ | REGION_KERNEL
    );

    if ( region == NULL ) {
        kprintf( WARNING, "mp_find_floating_pointer(): Failed to create memory region!\n" );
        return false;
    }

    do_memory_region_remap_pages( region, ( ptr_t )addr & PAGE_MASK );
    memory_context_insert_region( &kernel_memory_context, region );

    for ( offset = 0; offset < size; offset += 16 ) {
        fp = ( mp_floating_pointer_t* )( ( uint8_t* )addr + offset );

        if ( ( memcmp( fp->signature, mp_fp_signature, sizeof( mp_fp_signature ) ) == 0 ) &&
             ( mp_checksum( ( uint8_t* )fp, 16 ) == 0 ) ) {
            *_fp = fp;

            return true;
        }
    }

    /* TODO: MM put memory region without locking! */

    return false;
}

__init int init_mp( void ) {
    int i;
    int error;
    bool fp_found;
    mp_floating_pointer_t* fp;

    for ( i = 0; i < ARRAY_SIZE( mp_fp_places ); i += 2 ) {
        fp_found = mp_find_floating_pointer( ( void* )mp_fp_places[ i ], mp_fp_places[ i + 1 ], &fp );

        if ( fp_found ) {
            break;
        }
    }

    if ( !fp_found ) {
        kprintf( INFO, "MP floating pointer not found!\n" );
        return -ENOENT;
    }

    kprintf( INFO, "Found MP floating pointer at 0x%x\n", fp );

    if ( fp->mp_feature[ 0 ] == 0 ) {
        error = mp_handle_cfg_table( ( void* )fp->physical_address );

        if ( error < 0 ) {
            kprintf( ERROR, "Failed to parse MP configuration table!\n" );
            return error;
        }
    } else {
        mp_handle_default_cfg();
    }

    kprintf( INFO, "Found %d processors in the system.\n", processor_count );

    return 0;
}
