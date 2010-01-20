/* Programmable Interval Timer
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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

#include <irq.h>
#include <console.h>
#include <time.h>
#include <kernel.h>
#include <sched/scheduler.h>

#include <arch/pit.h>
#include <arch/io.h>
#include <arch/interrupt.h>
#include <arch/spinlock.h>
#include <arch/hwtime.h>
#include <arch/apic.h>
#include <arch/cpu.h>

static int pit_freq = 1000; /* PIT frequency in Hz */
static uint64_t boot_time = 0;

static int pit_irq( int irq, void* data, registers_t* regs ) {
    /* Wake up sleeper threads */

    waitqueue_wake_up( &sleep_queue, get_system_time() );

    if ( !apic_present ) {
        /* The PIT irq was acked and masked by the interrupt handler.
           We have to re-enable it here to let it fire next time. */

        arch_enable_irq( irq );

        schedule( regs );
    }

    return 0;
}

int pit_read_timer( void ) {
    int count;

    outb( 0, PIT_MODE );
    count = inb( PIT_CH0 );
    count |= ( inb( PIT_CH0 ) << 8 );

    return count;
}

void pit_wait_wrap( void ) {
    int delta;
    uint32_t prev = 0;
    uint32_t current = pit_read_timer();

    for ( ;; ) {
        prev = current;
        current = pit_read_timer();
        delta = current - prev;

        if ( delta > 300 ) {
            break;
        }
    }
}

uint64_t get_system_time( void ) {
    return ( ( rdtsc() * tsc_to_ns_scale ) >> CYC2NS_SCALE_FACTOR ) / 1000 + boot_time;
}

int set_system_time( time_t* newtime ) {
    /* todo */
    return 0;
}

uint64_t get_boot_time( void ) {
    return boot_time;
}

__init int init_system_time( void ) {
    tm_t now;

    gethwclock( &now );
    boot_time = 1000000 * mktime( &now );

    write_msr( X86_MSR_TSC, 0 );

    return 0;
}

__init int init_pit( void ) {
    int error;
    uint32_t base;

    if ( get_kernel_param_as_int( "pit_freq", &pit_freq ) == 0 ) {
        switch ( pit_freq ) {
            case 250 :
            case 500 :
            case 1000 :
                break;

            default :
                kprintf( WARNING, "pit: Invalid frequency from parameter. Using the default 1000 Hz.\n" );
                pit_freq = 1000;
                break;
        }
    }

    kprintf( INFO, "pit: Using frequency: %d Hz.\n", pit_freq );

    /* Set PIT frequency */

    base = PIT_TICKS_PER_SEC / pit_freq;
    outb( 0x36, PIT_MODE );
    outb( base & 0xFF, PIT_CH0 );
    outb( base >> 8, PIT_CH0 );

    /* Request the PIT irq */

    error = request_irq( 0, pit_irq, NULL );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
