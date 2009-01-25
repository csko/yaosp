/* Entry point of AP processors
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

.extern ap_stack_top
.extern ap_running

.section .smp_trampoline
.code16

ap_entry:
    cli

    movw %cs, %ax
    movw %ax, %ds
    movw %ax, %es

    lgdtl smp_gdtr - ap_entry

    /* Enable protected mode */

    movl %cr0, %eax
    orl $1, %eax
    movl %eax, %cr0

    /* Jump to the kernel's 32bit SMP entry point */

    ljmpl $KERNEL_CS, $1f

smp_gdt:
    .quad 0x0000000000000000

    .word 0xFFFF /* limit */
    .word 0x0000 /* base */
    .byte 0x00   /* base */
    .byte 0x9A   /* access */
    .byte 0xCF   /* granularity */
    .byte 0x00   /* base */

    .word 0xFFFF
    .word 0x0000
    .byte 0x00
    .byte 0x92
    .byte 0xCF
    .byte 0x00
smp_gdtr:
    .word smp_gdtr - smp_gdt - 1
    .long smp_gdt
.size smp_gdtr,.-smp_gdtr

.section .text
.code32

.align 16

1:
    movw $KERNEL_DS, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ss

    /* Load the stack allocated by the BSP */

    movl $ap_stack_top, %eax
    movl (%eax), %esp

    /* Tell the BSP that we're running */

    movl $ap_running, %eax
    lock
    incl (%eax)

    /* Enter the C part of the kernel */

    call ap_processor_entry

2:
    hlt
    jmp 2b
