/* i386 boot code
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

#include <multiboot.h>

.global _start

.section .multiboot_header

/* Multiboot header */

.long MB_HEADER_MAGIC /* magic */
.long ( MB_FLAG_ALIGN_MODULES | MB_FLAG_MEMORY_INFO ) /* flags */
.long -( MB_HEADER_MAGIC + ( MB_FLAG_ALIGN_MODULES | MB_FLAG_MEMORY_INFO ) ) /* checksum */

.section .text

/* The entry point of the kernel */

_start:
    /* Setup out own stack for the kernel */

    movl $_kernel_stack_top, %esp

    /* Check if the kernel was loaded with a multiboot
       compatible bootloader or not */

    cmpl $MB_BOOTLOADER_MAGIC, %eax
    jne _multiboot_nok

_multiboot_nok:
    hlt
    jmp _multiboot_nok

.section .bss

/* Kernel stack: 2048*4 = 8kB */

.rept 2047
.long 0
.endr
_kernel_stack_top:
.long 0
