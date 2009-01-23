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

#ifndef _ARCH_APIC_H_
#define _ARCH_APIC_H_

#include <types.h>

#define APIC_TIMER_IRQ    0xF0
#define APIC_SPURIOUS_IRQ 0xF1

/* Local APIC register offsets */

enum {
    LAPIC_ID = 0x20,
    LAPIC_VERSION = 0x30,
    LAPIC_TASK_PRIORITY = 0x80,
    LAPIC_EOI = 0xB0,
    LAPIC_SPURIOUS_INTERRUPT = 0xF0,
    LAPIC_ICR_LOW = 0x300,
    LAPIC_ICR_HIGH = 0x310,
    LAPIC_LVT_TIMER = 0x320,
    LAPIC_TIMER_INIT_COUNT = 0x380,
    LAPIC_TIMER_CURRENT_COUNT = 0x390,
    LAPIC_TIMER_DIVIDE = 0x3E0
};

/* Constants for the local APIC timer division factor */

enum {
    LAPIC_TIMER_DIV_1 = 0xB,
    LAPIC_TIMER_DIV_2 = 0x0,
    LAPIC_TIMER_DIV_4 = 0x1,
    LAPIC_TIMER_DIV_8 = 0x2,
    LAPIC_TIMER_DIV_16 = 0x3,
    LAPIC_TIMER_DIV_32 = 0x8,
    LAPIC_TIMER_DIV_64 = 0x9,
    LAPIC_TIMER_DIV_128 = 0xA
};

extern uint32_t local_apic_base;

extern int apic_to_logical_cpu_id[ 256 ];

void setup_local_apic( void );

int init_apic( void );

#endif // _ARCH_APIC_H_
