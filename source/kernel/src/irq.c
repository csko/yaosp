/* IRQ handling code
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

#include <irq.h>
#include <errno.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/interrupt.h>

static irq_action_t* irq_handlers[ ARCH_IRQ_COUNT ];

int request_irq( int irq, irq_handler_t* handler, void* data ) {
    bool enable_arch_irq;
    irq_action_t* action;

    if ( ( irq < 0 ) || ( irq >= ARCH_IRQ_COUNT ) ) {
        return -EINVAL;
    }

    action = ( irq_action_t* )kmalloc( sizeof( irq_action_t ) );

    if ( action == NULL ) {
        return -ENOMEM;
    }

    action->handler = handler;
    action->data = data;

    enable_arch_irq = ( irq_handlers[ irq ] == NULL );

    action->next = irq_handlers[ irq ];
    irq_handlers[ irq ] = action;

    if ( enable_arch_irq ) {
        arch_enable_irq( irq );
    }

    return 0;
}

void do_handle_irq( int irq, registers_t* regs ) {
    irq_action_t* action;

    action = irq_handlers[ irq ];

    while ( action != NULL ) {
        action->handler( irq, action->data, regs );

        action = action->next;
    }
}

__init int init_irq_handlers( void ) {
    memset(
        irq_handlers,
        0,
        sizeof( irq_handler_t* ) * ARCH_IRQ_COUNT
    );

    return 0;
}
