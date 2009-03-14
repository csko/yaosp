/* Architecture specific network definitions
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

.section .text

.global ip_checksum

ip_checksum:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    movl 16(%esp), %esi
    movl 20(%esp), %ecx
    xorl %eax, %eax
    xorl %ebx, %ebx
1:
    cmpl $1, %ecx
    jng 3f
    lodsw
    addl %eax, %ebx
    testl $0x80000000, %ebx
    jz 2f
    movl %ebx, %edx
    andl $0xFFFF, %ebx
    shrl $16, %edx
    addl %edx, %ebx
2:
    subl $2, %ecx
    jmp 1b
3:
    testl %ecx, %ecx
    jz 4f
    xorl %eax, %eax
    lodsb
    addl %eax, %ebx
4:
    testl $0xFFFF0000, %ebx
    jz 5f
    movl %ebx, %edx
    andl $0xFFFF, %ebx
    shrl $16, %edx
    addl %edx, %ebx
    jmp 4b
5:
    movl %ebx, %eax
    notw %ax
    popl %esi
    popl %ebx
    popl %ebp
    ret
.size ip_checksum,.-ip_checksum
