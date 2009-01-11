/* Interrupt specific functions
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

#ifndef _ARCH_INTERRUPT_H_
#define _ARCH_INTERRUPT_H_

#include <types.h>

#define ARCH_IRQ_COUNT 16

/**
 * This is used to disable interrupts on the current
 * processor.
 *
 * @return True is returned if interrupts were enabled before this call
 */
bool disable_interrupts( void );

/**
 * This is used to enable interrupts on the current processor.
 */
void enable_interrupts( void );

/**
 * Checks if interrupts are disabled or not.
 *
 * @return Returns true if interrupts are disabled
 */
bool is_interrupts_disabled( void );

void arch_disable_irq( int irq );
void arch_enable_irq( int irq );

/**
 * This is used during the kernel initialization to setup the
 * IDT entries and reprogram the PIC.
 *
 * @return On success 0 is returned
 */
int init_interrupts( void );

#endif // _ARCH_INTERRUPT_H_
