/* High Precision Event Timer
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

#include <errno.h>
#include <console.h>
#include <mm/region.h>
#include <mm/context.h>

#include <arch/hpet.h>
#include <arch/cpu.h>

int hpet_present = 0;
uint32_t hpet_address = 0;

static uint32_t hpet_period;

static uint32_t hpet_virt_address;
static memory_region_t* hpet_register_region;

uint32_t hpet_readl( uint32_t reg ) {
    return *( volatile uint32_t* )( hpet_virt_address + reg );
}

void hpet_writel( uint32_t value, uint32_t reg ) {
    *( volatile uint32_t* )( hpet_virt_address + reg ) = value;
}

static int hpet_stop_counter( void ) {
    uint32_t cfg;

    cfg = hpet_readl( HPET_CONFIG );
    cfg &= ~HPET_CONFIG_ENABLE;
    hpet_writel( cfg, HPET_CONFIG );

    return 0;
}

static int hpet_reset_counter( void ) {
    hpet_writel( 0, HPET_COUNTER );
    hpet_writel( 0, HPET_COUNTER + 4 );

    return 0;
}

static int hpet_start_counter( void ) {
    uint32_t cfg;

    cfg = hpet_readl( HPET_CONFIG );
    cfg |= HPET_CONFIG_ENABLE;
    hpet_writel( cfg, HPET_CONFIG );

    return 0;
}

static int hpet_restart_counter( void ) {
    hpet_stop_counter();
    hpet_reset_counter();
    hpet_start_counter();

    return 0;
}

int hpet_init( void ) {
    int i;
    uint32_t counter;
    uint64_t start, now;

    if ( !hpet_present ) {
        return -ENOENT;
    }

    /* Setup HPET mapping */

    kprintf( INFO, "HPET: Register area is at 0x%p.\n", hpet_address );

    hpet_register_region = do_create_memory_region( &kernel_memory_context, "HPET registers", PAGE_SIZE,
                                                    REGION_KERNEL | REGION_READ | REGION_WRITE );

    if ( hpet_register_region == NULL ) {
        return -ENOMEM;
    }

    do_memory_region_remap_pages( hpet_register_region, hpet_address );
    memory_context_insert_region( &kernel_memory_context, hpet_register_region );

    hpet_virt_address = hpet_register_region->address;

    /* Configure HPET */

    if ( hpet_readl( HPET_ID ) == 0 ) {
        kprintf( WARNING, "HPET: found invalid ID: 0.\n" );
        goto no_hpet;
    }

    hpet_period = hpet_readl( HPET_PERIOD );

    for ( i = 0; hpet_readl( HPET_CONFIG ) == 0xFFFFFFFF; i++ ) {
        if ( i == 1000 ) {
            kprintf( WARNING, "HPET: config register value is 0xFFFFFFFF.\n" );
            hpet_present = 0;
            goto no_hpet;
        }
    }

    if ( ( hpet_period < HPET_MIN_PERIOD ) ||
         ( hpet_period > HPET_MAX_PERIOD ) ) {
        goto no_hpet;
    }

    /* Verify that HPET counter works */

    hpet_restart_counter();

    counter = hpet_readl( HPET_COUNTER );
    start = rdtsc();

    do {
        now = rdtsc();
    } while ( ( now - start ) < 200000 );

    if ( counter == hpet_readl( HPET_COUNTER ) ) {
        kprintf( WARNING, "HPET: counter is not counting.\n" );
        goto no_hpet;
    }

    /* HPET seems to be okey.
       Insert its memory region to the kernel memory context. */

    return 0;

 no_hpet:
    hpet_present = 0;

    /* Release allocated resources. */

    do_memory_region_put( hpet_register_region );
    hpet_register_region = NULL;
    hpet_virt_address = 0;

    kprintf( INFO, "Disabling HPET.\n" );

    return -ENOENT;
}
