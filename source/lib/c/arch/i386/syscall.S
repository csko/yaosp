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

.global syscall1
.global syscall2

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
