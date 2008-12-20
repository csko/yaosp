/* IRQ handling code
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

#ifndef _IRQ_H_
#define _IRQ_H_

#include <types.h>

typedef int irq_handler_t( int irq, void* data, registers_t* regs );

typedef struct irq_action {
    irq_handler_t* handler;
    void* data;
    struct irq_action* next;
} irq_action_t;

int request_irq( int irq, irq_handler_t* handler, void* data );

void do_handle_irq( int irq, registers_t* regs );

int init_irq_handlers( void );

#endif // _IRQ_H_
