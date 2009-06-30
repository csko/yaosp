/* i386 architecture specific memcpy function
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

.global memcpy

.section .text

/* void* memcpy( void* d, const void* s, size_t n ) */

.type memcpy, @function
memcpy:
    pushl %esi
    pushl %edi
    movl 12(%esp), %edi
    movl 16(%esp), %esi
    movl 20(%esp), %ecx
    movl %ecx, %edx
    shrl $2, %ecx
    testl %ecx, %ecx
    jz 4f
    rep movsl
4:
    movl %edx, %ecx
    andl $3, %ecx
    testl %ecx, %ecx
    jz 5f
    rep movsb
5:
    movl 12(%esp), %eax
    popl %edi
    popl %esi
    ret
.size memcpy,.-memcpy
