/* Advanced programmable interrupt controller
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#include <arch/apic.h>
#include <arch/cpu.h>

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
