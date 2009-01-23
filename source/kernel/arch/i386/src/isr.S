/* i386 specific interrupt service routines
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

#include <arch/gdt.h>

.section .text

#define ISR(num,handler) \
    .global isr##num; \
    isr##num:; \
    pushl $0; \
    pushl $##num; \
    pusha; \
    push %ds; \
    andl $0xFFFF, (%esp); \
    push %es; \
    andl $0xFFFF, (%esp); \
    push %fs; \
    andl $0xFFFF, (%esp); \
    mov $KERNEL_DS, %ax; \
    mov %ax, %ds; \
    mov %ax, %es; \
    mov %ax, %fs; \
    pushl %esp; \
    call handler; \
    addl $4, %esp; \
    pop %fs; \
    pop %es; \
    pop %ds; \
    popa; \
    addl $8, %esp; \
    iret

#define ISR_WITH_ERROR(num,handler) \
    .global isr##num; \
    isr##num:; \
    pushl $##num; \
    pusha; \
    push %ds; \
    andl $0xFFFF, (%esp); \
    push %es; \
    andl $0xFFFF, (%esp); \
    push %fs; \
    andl $0xFFFF, (%esp); \
    mov $KERNEL_DS, %ax; \
    mov %ax, %ds; \
    mov %ax, %es; \
    mov %ax, %fs; \
    pushl %esp; \
    call handler; \
    addl $4, %esp; \
    pop %fs; \
    pop %es; \
    pop %ds; \
    popa; \
    addl $8, %esp; \
    iret

/* exception handlers */

ISR(0,handle_division_by_zero)
ISR(1,handle_debug_exception)
ISR(3,handle_debug_exception)
ISR(6,handle_invalid_opcode)
ISR(7,handle_device_not_available)
ISR_WITH_ERROR(13,handle_general_protection_fault)
ISR_WITH_ERROR(14,handle_page_fault)
ISR(16,handle_fpu_exception)
ISR(19,handle_sse_exception)

/* interrupt handlers */

ISR(32,irq_handler)
ISR(33,irq_handler)
ISR(34,irq_handler)
ISR(35,irq_handler)
ISR(36,irq_handler)
ISR(37,irq_handler)
ISR(38,irq_handler)
ISR(39,irq_handler)
ISR(40,irq_handler)
ISR(41,irq_handler)
ISR(42,irq_handler)
ISR(43,irq_handler)
ISR(44,irq_handler)
ISR(45,irq_handler)
ISR(46,irq_handler)
ISR(47,irq_handler)

ISR(128,system_call_entry)

ISR(240,apic_timer_irq)
ISR(241,apic_spurious_irq)
