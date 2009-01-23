/* Advanced programmable interrupt controller
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

#include <types.h>
#include <errno.h>
#include <console.h>
#include <mm/region.h>

#include <arch/apic.h>
#include <arch/cpu.h>

uint32_t local_apic_base = 0;
region_id local_apic_region;

static int fake_lapic_id = 0;
static uint32_t local_apic_address = ( uint32_t )&fake_lapic_id - LAPIC_ID;

int apic_to_logical_cpu_id[ 256 ] = { 0, };

static inline uint32_t apic_read( uint32_t reg ) {
    return *( ( volatile uint32_t* )( local_apic_address + reg ) );
}

static inline void apic_write( uint32_t reg, uint32_t value ) {
    *( ( volatile uint32_t* )( local_apic_address + reg ) ) = value;
}

int get_processor_id( void ) {
    register int id;

    id = apic_read( LAPIC_ID ) >> 24;

    switch ( arch_processor_table[ 0 ].family ) {
        case 4 :
        case 5 :
        case 6 :
            id &= 0x0F;
            break;

        default :
            id &= 0xFF;
            break;
    }

    return id;
}

cpu_t* get_processor( void ) {
    register int id;
    register int logical_id;

    id = get_processor_id();
    logical_id = apic_to_logical_cpu_id[ id ];

    return &processor_table[ logical_id ];
}

void apic_timer_irq( registers_t* regs ) {
    kprintf( "APIC timer IRQ!\n" );
}

void apic_spurious_irq( registers_t* regs ) {
    kprintf( "APIC spurious IRQ!\n" );
}

void setup_local_apic( void ) {
    /* Set Task Priority to "accept all" interrupts */

    apic_write( LAPIC_TASK_PRIORITY, 0 );

    /* Enable the local APIC and set the spurious interrupt vector */

    apic_write(
        LAPIC_SPURIOUS_INTERRUPT,
        ( 1 << 8 ) | /* enable APIC */
        APIC_SPURIOUS_IRQ /* spurious irq */
    );

    /* Clear pending interrupts (if there is any) */

    apic_write( LAPIC_EOI, 0 );
}

int init_apic( void ) {
    int error;

    /* First check if we found the APIC in the system */

    if ( local_apic_base == 0 ) {
        return -ENOENT;
    }

    kprintf( "Local APIC address: 0x%x\n", local_apic_base );

    /* Create a memory region for the local APIC registers */

    local_apic_region = do_create_region(
        "local APIC registers",
        PAGE_SIZE,
        REGION_READ | REGION_WRITE | REGION_KERNEL,
        ALLOC_NONE,
        ( void** )&local_apic_address
    );

    if ( local_apic_region < 0 ) {
        kprintf( "Failed to create memory region for local APIC registers!\n" );
        return local_apic_region;
    }

    /* Remap the created region */

    error = do_remap_region( local_apic_region, ( ptr_t )local_apic_base );

    if ( error < 0 ) {
        kprintf( "Failed to remap the local APIC register memory region!\n" );
        /* TODO: delete region */
        return error;
    }

    setup_local_apic();

    return 0;
}
