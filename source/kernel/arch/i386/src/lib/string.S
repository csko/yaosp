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

/* void* memset( void* d, int c, size_t n ) */

memset:
    pushl %ecx
    pushl %edi
    movl 20(%esp), %ecx
    testl %ecx, %ecx
    jz 1f
    movl 12(%esp), %edi
    movl 16(%esp), %eax
    rep stosb
1:
    movl 12(%esp), %eax
    popl %edi
    popl %ecx
    ret
.size memset,.-memset

/* void* memsetw( void* d, int c, size_t n ) */

memsetw:
    pushl %ecx
    pushl %edi
    movl 20(%esp), %ecx
    testl %ecx, %ecx
    jz 2f
    movl 12(%esp), %edi
    movl 16(%esp), %eax
    rep stosw
2:
    movl 12(%esp), %eax
    popl %edi
    popl %ecx
    ret
.size memsetw,.-memsetw

/* void* memsetl( void* d, int c, size_t n ) */

memsetl:
    pushl %ecx
    pushl %edi
    movl 20(%esp), %ecx
    testl %ecx, %ecx
    jz 3f
    movl 12(%esp), %edi
    movl 16(%esp), %eax
    rep stosl
3:
    movl 12(%esp), %eax
    popl %edi
    popl %ecx
    ret
.size memsetl,.-memsetl
