/* Interrupt handler functions
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

#include <types.h>
#include <console.h>
#include <irq.h>
#include <kernel.h>
#include <lib/string.h>

#include <arch/idt.h>
#include <arch/interrupt.h>
#include <arch/gdt.h>
#include <arch/io.h>
#include <arch/apic.h>

#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_IMR 0x21
#define PIC_SLAVE_CMD  0xA0
#define PIC_SLAVE_IMR  0xA1
#define PIC_CASCADE_IR 0x02

#define MASTER_ICW4_DEFAULT 0x01
#define SLAVE_ICW4_DEFAULT  0x01

extern void isr0( void );
extern void isr1( void );
extern void isr3( void );
extern void isr6( void );
extern void isr7( void );
extern void isr13( void );
extern void isr14( void );
extern void isr16( void );
extern void isr19( void );

extern void isr32( void );
extern void isr33( void );
extern void isr34( void );
extern void isr35( void );
extern void isr36( void );
extern void isr37( void );
extern void isr38( void );
extern void isr39( void );
extern void isr40( void );
extern void isr41( void );
extern void isr42( void );
extern void isr43( void );
extern void isr44( void );
extern void isr45( void );
extern void isr46( void );
extern void isr47( void );
extern void isr128( void );
extern void isr240( void );
extern void isr241( void );
extern void isr242( void );

idt_descriptor_t idt[ IDT_ENTRIES ];

static uint8_t master_mask = 0xFF;
static uint8_t slave_mask = 0xFF;

static void set_trap_gate( int num, void* handler) {
    idt[ num ].selector = KERNEL_CS;
    idt[ num ].base_low = ( ( uint32_t )handler ) & 0xFFFF;
    idt[ num ].base_high = ( ( uint32_t )handler ) >> 16;
    idt[ num ].flags = 0xEF;
}

static void set_interrupt_gate( int num, void* handler ) {
    idt[ num ].selector = KERNEL_CS;
    idt[ num ].base_low = ( ( uint32_t )handler ) & 0xFFFF;
    idt[ num ].base_high = ( ( uint32_t )handler ) >> 16;
    idt[ num ].flags = 0x8E;
}

void arch_disable_irq( int irq ) {
    if ( irq & 8 ) {
        slave_mask |= ( 1 << ( irq - 8 ) );
        outb( slave_mask, PIC_SLAVE_IMR );
    } else {
        master_mask |= ( 1 << irq );
        outb( master_mask, PIC_MASTER_IMR );
    }
}

void arch_enable_irq( int irq ) {
    if ( irq & 8 ) {
        slave_mask &= ~( 1 << ( irq - 8 ) );
        outb( slave_mask , PIC_SLAVE_IMR );
    } else {
        master_mask &= ~( 1 << irq );
        outb( master_mask, PIC_MASTER_IMR );
    }
}

void arch_mask_and_ack_irq( int irq ) {
  if ( irq & 8 ) {
    slave_mask |= ( 1 << ( irq - 8 ) );
    inb( PIC_SLAVE_IMR );
    outb( slave_mask, PIC_SLAVE_IMR );
    outb( 0x60 + ( irq & 7 ), PIC_SLAVE_CMD );
    outb( 0x60 + PIC_CASCADE_IR, PIC_MASTER_CMD );
  } else {
    master_mask |= ( 1 << irq );
    inb( PIC_MASTER_IMR );
    outb( master_mask, PIC_MASTER_IMR );
    outb( 0x60 + irq, PIC_MASTER_CMD );
  }
}

void irq_handler( registers_t* regs ) {
    int irq;

    irq = regs->int_number - 0x20;

    arch_mask_and_ack_irq( irq );
    do_handle_irq( irq, regs );
    arch_enable_irq( irq );
}

__init int init_interrupts( void ) {
    idt_t idtp;

    /* Zero the whole Interrupt Descriptor Table */

    memset(
        idt,
        0,
        sizeof( idt_descriptor_t ) * IDT_ENTRIES
    );

    set_trap_gate( 0, isr0 );
    set_interrupt_gate( 1, isr1 );
    set_trap_gate( 3, isr3 );
    set_trap_gate( 6, isr6 );
    set_trap_gate( 7, isr7 );
    set_trap_gate( 13, isr13 );
    set_trap_gate( 14, isr14 );
    set_trap_gate( 16, isr16 );
    set_trap_gate( 19, isr19 );

    set_interrupt_gate( 32, isr32 );
    set_interrupt_gate( 33, isr33 );
    set_interrupt_gate( 34, isr34 );
    set_interrupt_gate( 35, isr35 );
    set_interrupt_gate( 36, isr36 );
    set_interrupt_gate( 37, isr37 );
    set_interrupt_gate( 38, isr38 );
    set_interrupt_gate( 39, isr39 );
    set_interrupt_gate( 40, isr40 );
    set_interrupt_gate( 41, isr41 );
    set_interrupt_gate( 42, isr42 );
    set_interrupt_gate( 43, isr43 );
    set_interrupt_gate( 44, isr44 );
    set_interrupt_gate( 45, isr45 );
    set_interrupt_gate( 46, isr46 );
    set_interrupt_gate( 47, isr47 );

    set_trap_gate( 0x80, isr128 );

    set_interrupt_gate( APIC_TIMER_IRQ, isr240 );
    set_interrupt_gate( APIC_SPURIOUS_IRQ, isr241 );
    set_interrupt_gate( APIC_TLB_FLUSH_IRQ, isr242 );

    idtp.limit = ( sizeof( idt_descriptor_t ) * IDT_ENTRIES ) - 1;
    idtp.base = ( uint32_t )&idt;

    /* Load the IDT */

    __asm__ __volatile__(
        "lidt %0\n"
        :
        : "m" ( idtp )
    );

    /* Reprogram the Programmable Interrupt Controller */

    outb( 0xFF, PIC_SLAVE_IMR );
    outb( 0xFF, PIC_MASTER_IMR );

    outb( 0x11, PIC_MASTER_CMD );
    outb( 0x20, PIC_MASTER_IMR );
    outb( 0x04, PIC_MASTER_IMR );
    outb( MASTER_ICW4_DEFAULT, PIC_MASTER_IMR );

    outb( 0x11, PIC_SLAVE_CMD );
    outb( 0x28, PIC_SLAVE_IMR );
    outb( 0x02, PIC_SLAVE_IMR );
    outb( SLAVE_ICW4_DEFAULT, PIC_SLAVE_IMR );

    master_mask &= ~( 1 << 2 ); /* enable IRQ 2 */

    /* Write interrupt masks to the PIC */

    outb( slave_mask, PIC_SLAVE_IMR );
    outb( master_mask, PIC_MASTER_IMR );

    return 0;
}
