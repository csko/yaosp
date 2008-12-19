/* Atomic operations
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

.global atomic_get
.global atomic_set
.global atomic_inc
.global atomic_dec
.global atomic_swap

/* int atomic_get( atomic_t* atomic ); */

atomic_get:
    movl 4(%esp), %edx
    lock
    movl (%edx), %eax
    ret
.size atomic_get,.-atomci_get

/* int atomic_set( atomic_t* atomic, int value ); */

atomic_set:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    lock
    movl %eax, (%edx)
    ret
.size atomic_set,.-atomic_set

/* int atomic_inc( atomic_t* atomic ); */

atomic_inc:
    movl 4(%esp), %edx
    lock
    incl (%edx)
    ret
.size atomic_inc,.-atomic_inc

/* int atomic_dec( atomic_t* atomic ); */

atomic_dec:
    movl 4(%esp), %edx
    lock
    decl (%edx)
    ret
.size atomic_dec,.-atomic_dec

/* int atomic_swap( atomic_t* atomic, int value ); */

atomic_swap:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    lock
    xchgl %eax, (%edx)
    ret
.size atomic_swap,.-atomic_swap
