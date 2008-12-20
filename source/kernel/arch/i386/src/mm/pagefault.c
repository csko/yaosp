/* Page fault handler
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
#include <console.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>

void handle_page_fault( registers_t* regs ) {
    register_t address;

    kprintf( "Page fault!\n" );

    __asm__ __volatile__(
        "movl %%cr2, %0\n"
        : "=r" ( address )
    );

    kprintf( "Address=0x%x\n", address );

    disable_interrupts();
    halt_loop();
}
