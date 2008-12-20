/* Architecture specific type definitions
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

#ifndef _ARCH_TYPES_H_
#define _ARCH_TYPES_H_

typedef signed long long int int64_t;

typedef unsigned long long int uint64_t;

typedef unsigned int ptr_t;
typedef unsigned int register_t;

typedef struct regsisters {
    /* Saved segment registers */
    register_t fs;
    register_t es;
    register_t ds;
    /* General-purpose registers pushed by the pusha
       instruction */
    register_t edi;
    register_t esi;
    register_t ebp;
    register_t ksp;
    register_t ebx;
    register_t edx;
    register_t ecx;
    register_t eax;
    /* Interrupt number and error code pushed by the
       ISR macros */
    register_t int_number;
    register_t error_code;
    /* These are pushed to the stack by the CPU when
       an interrupt is fired */
    register_t eip;
    register_t cs;
    register_t eflags;
    register_t esp;
    register_t ss;
} registers_t;

#endif // _ARCH_TYPES_H_
