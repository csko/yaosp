/* i386 architecture specific processor definitions
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

#ifndef _ARCH_CPU_H_
#define _ARCH_CPU_H_

#define EFLAG_IF ( 1 << 9 )  /* Interrupt flag */
#define EFLAG_ID ( 1 << 21 ) /* ID flag */

#ifndef __ASSEMBLER__

#include <types.h>
#include <config.h>

typedef struct i386_cpu {
    int family;
    int model;
    int features;
} i386_cpu_t;

typedef struct i386_feature {
    int feature;
    const char* name;
} i386_feature_t;

enum {
    CPU_FEATURE_MMX = ( 1 << 0 ),
    CPU_FEATURE_SSE = ( 1 << 1 ),
    CPU_FEATURE_APIC = ( 1 << 2 ),
    CPU_FEATURE_MTRR = ( 1 << 3 ),
    CPU_FEATURE_SYSCALL = ( 1 << 4 ),
    CPU_FEATURE_TSC = ( 1 << 5 )
};

extern i386_cpu_t arch_processor_table[ MAX_CPU_COUNT ];
extern i386_feature_t i386_features[];

register_t get_eflags( void );
void set_eflags( register_t eflags );

int detect_cpu( void );

#endif // __ASSELBLER__

#endif // _ARCH_CPU_H_
