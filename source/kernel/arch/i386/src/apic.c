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
#include <scheduler.h>
#include <config.h>
#include <mm/region.h>

#include <arch/apic.h>
#include <arch/cpu.h>
#include <arch/pit.h>
#include <arch/io.h>
#include <arch/smp.h>
#include <arch/atomic.h>

static int fake_lapic_id = 0;

bool apic_present = false;

uint32_t local_apic_base = 0;
region_id local_apic_region;
uint32_t local_apic_address = ( uint32_t )&fake_lapic_id - LAPIC_ID;

#ifdef ENABLE_SMP
int apic_to_logical_cpu_id[ 256 ] = { 0, };

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
#else
int get_processor_id( void ) {
    return 0;
}

cpu_t* get_processor( void ) {
    return &processor_table[ 0 ];
}
#endif /* ENABLE_SMP */

void apic_timer_irq( registers_t* regs ) {
    apic_write( LAPIC_EOI, 0 );

    ( ( volatile char* )0xB8000 )[ get_processor_id() * 2 ]++;

    schedule( regs );
}

void apic_spurious_irq( registers_t* regs ) {
    kprintf( "APIC spurious IRQ!\n" );
    apic_write( LAPIC_EOI, 0 );
}

void apic_tlb_flush_irq( registers_t* regs ) {
#ifdef ENABLE_SMP
    int processor_id;

    processor_id = get_processor_id();

    if ( atomic_test_and_clear( ( void* )&tlb_invalidate_mask, processor_id ) ) {
        flush_tlb();
    }
#endif /* ENABLE_SMP */

    apic_write( LAPIC_EOI, 0 );
}

void calibrate_apic_timer( void ) {
    int processor_id;
    uint32_t end_count;

    /* Divide the bus frequency with 1 */

    apic_write(
        LAPIC_TIMER_DIVIDE,
        LAPIC_TIMER_DIV_1
    );

    /* Configure PIT to use for the calibration */

    outb( PIT_MODE, 0x34 );
    outb( PIT_CH0, 0xFF );
    outb( PIT_CH0, 0xFF );

    pit_wait_wrap();

    /* Set the initial count of the timer to 0xFFFFFFFF */

    apic_write(
        LAPIC_TIMER_INIT_COUNT,
        0xFFFFFFFF
    );

    pit_wait_wrap();

    /* Read the current counter from the APIC register */

    end_count = apic_read( LAPIC_TIMER_CURRENT_COUNT );

    /* Calculate the bus speed */

    processor_id = get_processor_id();

    arch_processor_table[ processor_id ].bus_speed = ( ( uint64_t )PIT_TICKS_PER_SEC * ( 0xFFFFFFFFLL - end_count ) / 0xFFFF );

    kprintf(
        "CPU %d bus speed: %u MHz.\n",
        processor_id,
        ( uint32_t )( arch_processor_table[ processor_id ].bus_speed / 1000000 )
    );
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

    /* Setup the local APIC */

    setup_local_apic();

    /* Calibrate the APIC bus speed */

    calibrate_apic_timer();

    apic_present = true;

    return 0;
}

int init_apic_timer( void ) {
    uint32_t init_count;

    if ( !apic_present ) {
        return -ENOENT;
    }

    init_count = arch_processor_table[ 0 ].bus_speed;
    init_count /= 1000;
    init_count /= 4;

    apic_write( LAPIC_TIMER_DIVIDE, LAPIC_TIMER_DIV_4 );
    apic_write(
        LAPIC_LVT_TIMER,
        ( 1 << 17 ) | /* periodic timer */
        APIC_TIMER_IRQ
    );
    apic_write( LAPIC_TIMER_INIT_COUNT, init_count );

    return 0;
}
