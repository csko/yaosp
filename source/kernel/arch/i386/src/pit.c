/* Programmable Interval Timer
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

static uint64_t system_time = 0;
static uint64_t boot_time = 0;
static spinlock_t time_lock = INIT_SPINLOCK( "PIT" );

static int pit_irq( int irq, void* data, registers_t* regs ) {
    /* Increment the system time */

    spinlock( &time_lock );

    system_time += 1000;

    spinunlock( &time_lock );

    /* Wake up sleeper threads */

    spinlock( &scheduler_lock );

    waitqueue_wake_up( &sleep_queue, system_time );

    spinunlock( &scheduler_lock );

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
    uint64_t now;

    spinlock_disable( &time_lock );

    now = system_time;

    spinunlock_enable( &time_lock );

    return now;
}

int set_system_time( time_t* newtime ) {
    spinlock_disable( &time_lock );

    system_time = *newtime;

    spinunlock_enable( &time_lock );

    return 0;
}

uint64_t get_boot_time( void ) {
    return boot_time;
}

__init int init_system_time( void ) {
    tm_t now;
    uint64_t i;

    gethwclock( &now );
    i = 1000000 * mktime( &now );

    spinlock_disable( &time_lock );

    system_time = i;
    boot_time = i;

    spinunlock_enable( &time_lock );

    return 0;
}

__init int init_pit( void ) {
    int error;
    uint32_t base;

    /* Set frequency (1000Hz) */

    base = PIT_TICKS_PER_SEC / 1000;
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
