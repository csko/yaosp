/* i386 processor specific functions
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

#include <arch/cpu.h>
#include <arch/gdt.h>

.section .text

.global get_eflags
.global set_eflags
.global disable_interrupts
.global enable_interrupts
.global reload_segment_descriptors
.global get_cr2
.global get_cr3
.global set_cr3
.global flush_tlb
.global halt_loop

/* register_t get_eflags( void ) */

get_eflags:
    pushfl
    popl %eax
    ret
.size get_eflags,.-get_eflags

/* void set_eflags( register_t eflags ) */

set_eflags:
    pushl 4(%esp)
    popfl
    ret
.size set_eflags,.-set_eflags

/* bool disable_interrupts( void ) */

disable_interrupts:
    pushfl
    popl %eax
    cli
    andl $EFLAG_IF, %eax
    ret
.size disable_interrupts,.-disable_interrupts

/* void enable_interrupts( void ) */

enable_interrupts:
    sti
    ret
.size enable_interrupts,.-enable_interrupts

/* void reload_segment_descriptors( void ) */

reload_segment_descriptors:
    movw $KERNEL_DS, %ax
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ds
    movw %ax, %ss
    ljmpl $KERNEL_CS, $1f
1:
    ret
.size reload_segment_descriptors,.-reload_segment_descriptors

/* register_t get_cr2( void ) */

get_cr2:
    movl %cr2, %eax
    ret
.size get_cr2,.-get_cr2

/* register_t get_cr3( void ) */

get_cr3:
    movl %cr3, %eax
    ret
.size get_cr3,.-get_cr3

/* void set_cr3( uint32_t cr3 ) */

set_cr3:
    movl 4(%esp), %eax
    movl %eax, %cr3
    ret
.size set_cr3,.-set_cr3

/* void flush_tlb( void ) */

flush_tlb:
    movl %cr3, %eax
    movl %eax, %cr3
    ret
.size flush_tlb,.-flush_tlb

/* void halt_loop( void ) */

halt_loop:
    hlt
    jmp halt_loop
.size halt_loop,.-halt_loop
