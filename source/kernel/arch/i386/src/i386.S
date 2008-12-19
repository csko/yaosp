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

.global disable_interrupts
.global enable_interrupts
.global reload_segment_descriptors

disable_interrupts:
    pushfl
    popl %eax
    cli
    andl $EFLAG_IF, %eax
    ret
.size disable_interrupts,.-disable_interrupts

enable_interrupts:
    sti
    ret
.size enable_interrupts,.-enable_interrupts

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
