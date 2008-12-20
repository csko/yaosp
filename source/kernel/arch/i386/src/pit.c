/* Programmable Interval Timer
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

#include <irq.h>
#include <console.h>

#include <arch/pit.h>
#include <arch/io.h>

static int pit_irq( int irq, void* data, registers_t* regs ) {
    return 0;
}

int init_pit( void ) {
    uint32_t base;

    /* Set frequency (1000Hz) */

    base = 1193182L / 1000;
    outb( 0x36, 0x43 );
    outb( base & 0xFF, 0x40 );
    outb( base >> 8, 0x40 );

    /* Request the PIT irq */

    return request_irq( 0, pit_irq, NULL );
}
