/* i386 architecture specific string functions
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

.section .text

.global memset
.global memsetw
.global memsetl
.global memcpy

/* void* memset( void* d, int c, size_t n ) */

memset:
    pushl %edi
    movl 16(%esp), %ecx
    testl %ecx, %ecx
    jz 1f
    movl 8(%esp), %edi
    movl 12(%esp), %eax
    rep stosb
1:
    movl 8(%esp), %eax
    popl %edi
    ret
.size memset,.-memset

/* void* memsetw( void* d, int c, size_t n ) */

memsetw:
    pushl %edi
    movl 16(%esp), %ecx
    testl %ecx, %ecx
    jz 2f
    movl 8(%esp), %edi
    movl 12(%esp), %eax
    rep stosw
2:
    movl 8(%esp), %eax
    popl %edi
    ret
.size memsetw,.-memsetw

/* void* memsetl( void* d, int c, size_t n ) */

memsetl:
    pushl %edi
    movl 16(%esp), %ecx
    testl %ecx, %ecx
    jz 3f
    movl 8(%esp), %edi
    movl 12(%esp), %eax
    rep stosl
3:
    movl 8(%esp), %eax
    popl %edi
    ret
.size memsetl,.-memsetl

/* void* memcpy( void* d, const void* s, size_t n ) */

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
    rep stosb
5:
    movl 12(%esp), %eax
    popl %edi
    popl %esi
    ret
.size memcpy,.-memcpy
