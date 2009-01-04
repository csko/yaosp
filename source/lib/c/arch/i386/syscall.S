/* i386 architecture specific system call functions
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

.global syscall1
.global syscall2
.global syscall3
.global syscall4
.global syscall5

syscall1:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    movl 8(%ebp), %eax
    movl 12(%ebp), %ebx
    int $0x80
    popl %ebx
    popl %ebp
    ret

syscall2:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    movl 8(%ebp), %eax
    movl 12(%ebp), %ebx
    movl 16(%ebp), %ecx
    int $0x80
    popl %ebx
    popl %ebp
    ret

syscall3:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    movl 8(%ebp), %eax
    movl 12(%ebp), %ebx
    movl 16(%ebp), %ecx
    movl 20(%ebp), %edx
    int $0x80
    popl %ebx
    popl %ebp
    ret

syscall4:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    movl 8(%ebp), %eax
    movl 12(%ebp), %ebx
    movl 16(%ebp), %ecx
    movl 20(%ebp), %edx
    movl 24(%ebp), %esi
    int $0x80
    popl %esi
    popl %ebx
    popl %ebp
    ret

syscall5:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %esi
    pushl %edi
    movl 8(%ebp), %eax
    movl 12(%ebp), %ebx
    movl 16(%ebp), %ecx
    movl 20(%ebp), %edx
    movl 24(%ebp), %esi
    movl 28(%ebp), %edi
    int $0x80
    popl %edi
    popl %esi
    popl %ebx
    popl %ebp
    ret
