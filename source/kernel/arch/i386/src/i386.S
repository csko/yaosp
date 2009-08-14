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
.global is_interrupts_disabled
.global reload_segment_descriptors
.global get_cr2
.global set_cr2
.global get_cr3
.global set_cr3
.global get_ebp
.global clear_task_switched
.global set_task_switched
.global flush_tlb
.global invlpg
.global halt_loop

/* register_t get_eflags( void ) */

.type get_eflags, @function
get_eflags:
    pushfl
    popl %eax
    ret
.size get_eflags,.-get_eflags

/* void set_eflags( register_t eflags ) */

.type set_eflags, @function
set_eflags:
    pushl 4(%esp)
    popfl
    ret
.size set_eflags,.-set_eflags

/* bool disable_interrupts( void ) */

.type disable_interrupts, @function
disable_interrupts:
    pushfl
    popl %eax
    cli
    andl $EFLAG_IF, %eax
    ret
.size disable_interrupts,.-disable_interrupts

/* void enable_interrupts( void ) */

.type enable_interrupts, @function
enable_interrupts:
    sti
    ret
.size enable_interrupts,.-enable_interrupts

/* bool is_interrupts_disabled( void ) */

.type is_interrupts_disabled, @function
is_interrupts_disabled:
    pushfl
    popl %eax
    negl %eax
    andl $EFLAG_IF, %eax
    ret
.size is_interrupts_disabled,.-is_interrupts_disabled

/* register_t get_cr2( void ) */

.type get_cr2, @function
get_cr2:
    movl %cr2, %eax
    ret
.size get_cr2,.-get_cr2

/* void set_cr2( register_t ) */

.type set_cr2, @function
set_cr2:
    movl 4(%esp), %eax
    movl %eax, %cr2
    ret
.size set_cr2,.-set_cr2

/* register_t get_cr3( void ) */

.type get_cr3, @function
get_cr3:
    movl %cr3, %eax
    ret
.size get_cr3,.-get_cr3

/* void set_cr3( register_t cr3 ) */

.type set_cr3, @function
set_cr3:
    movl 4(%esp), %eax
    movl %eax, %cr3
    ret
.size set_cr3,.-set_cr3

/* register_t get_ebp( void ) */

.type get_ebp, @function
get_ebp:
    movl %ebp, %eax
    ret
.size get_ebp,.-get_ebp

/* void clear_task_switched( void ) */

.type clear_task_switched, @function
clear_task_switched:
    clts
    ret
.size clear_task_switched,.-clear_task_switched

/* void set_task_switched( void ) */

.type set_task_switched, @function
set_task_switched:
    movl %cr0, %eax
    orl $0x8, %eax
    movl %eax, %cr0
    ret
.size set_task_switched,.-set_task_switched

/* void flush_tlb( void ) */

.type flush_tlb, @function
flush_tlb:
    movl %cr3, %eax
    movl %eax, %cr3
    ret
.size flush_tlb,.-flush_tlb

/* void invlpg( uint32_t address ) */
.type invlpg, @function
invlpg:
    invlpg 4(%esp)
    ret
.size invlpg,.-invlpg

/* void halt_loop( void ) */

.type halt_loop, @function
halt_loop:
    hlt
    jmp halt_loop
.size halt_loop,.-halt_loop

.section .kernel_init

/* void reload_segment_descriptors( void ) */

.type reload_segment_descriptors, @function
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
