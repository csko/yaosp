/* Atomic operations
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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
.global atomic_dec_and_test
.global atomic_swap
.global atomic_test_and_clear

/* int atomic_get( atomic_t* atomic ); */

.type atomic_get, @function
atomic_get:
    movl 4(%esp), %edx
    movl (%edx), %eax
    ret
.size atomic_get,.-atomic_get

/* int atomic_set( atomic_t* atomic, int value ); */

.type atomic_set, @function
atomic_set:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    movl %eax, (%edx)
    ret
.size atomic_set,.-atomic_set

/* void atomic_inc( atomic_t* atomic ); */

.type atomic_inc, @function
atomic_inc:
    movl 4(%esp), %edx
    lock
    incl (%edx)
    ret
.size atomic_inc,.-atomic_inc

/* void atomic_dec( atomic_t* atomic ); */

.type atomic_dec, @function
atomic_dec:
    movl 4(%esp), %edx
    lock
    decl (%edx)
    ret
.size atomic_dec,.-atomic_dec

/* bool atomic_dec_and_test( atomic_t* atomic ); */

.type atomic_dec_and_test, @function
atomic_dec_and_test:
    movl 4(%esp), %edx
    movl $0, %eax
    lock
    decl (%edx)
    setz %al
    ret
.size atomic_dec_and_test,.-atomic_dec_and_test

/* int atomic_swap( atomic_t* atomic, int value ); */

.type atomic_swap, @function
atomic_swap:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    lock
    xchgl %eax, (%edx)
    ret
.size atomic_swap,.-atomic_swap

/* int atomic_test_and_clear( void* address, int bit ); */

.type atomic_test_and_clear, @function
atomic_test_and_clear:
    movl 4(%esp), %edx
    movl 8(%esp), %eax
    lock
    btrl %eax, (%edx)
    sbbl %eax, %eax
    ret
.size atomic_test_and_clear,.-atomic_test_and_clear
