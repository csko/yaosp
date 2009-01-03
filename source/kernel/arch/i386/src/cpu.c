/* Processor detection code
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

#include <errno.h>
#include <types.h>
#include <console.h>
#include <smp.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/gdt.h>

i386_cpu_t arch_processor_table[ MAX_CPU_COUNT ];

i386_feature_t i386_features[] = {
    { CPU_FEATURE_MMX, "mmx" },
    { CPU_FEATURE_SSE, "sse" },
    { CPU_FEATURE_APIC, "apic" },
    { CPU_FEATURE_MTRR, "mtrr" },
    { CPU_FEATURE_SYSCALL, "syscall" },
    { CPU_FEATURE_TSC, "tsc" },
    { 0, "" }
};

static inline void cpuid( uint32_t reg, register_t* data ) {
    __asm__ __volatile__(
        "cpuid\n"
        : "=a" ( data[ 0 ] ), "=b" ( data[ 1 ] ), "=c" ( data[ 2 ] ), "=d" ( data[ 3 ] )
        : "0" ( reg )
    );
}

static bool cpuid_supported( void ) {
    register_t old_eflags;
    register_t new_eflags;

    old_eflags = get_eflags();
    set_eflags( old_eflags ^ EFLAG_ID );
    new_eflags = get_eflags();

    return ( ( old_eflags & EFLAG_ID ) != ( new_eflags & EFLAG_ID ) );
}

int detect_cpu( void ) {
    int i;
    int family = 0;
    int model = 0;
    int features = 0;
    register_t regs[ 4 ];
    register_t largest_func_num;
    char name[ 49 ] = { 0 };

    /* Clear the processor structures */

    memset( processor_table, 0, sizeof( cpu_t ) * MAX_CPU_COUNT );
    //memset( arch_processor_table, 0, sizeof( i386_cpu_t ) * MAX_CPU_COUNT );

    if ( !cpuid_supported() ) {
        return -EINVAL;
    }

    cpuid( 0, regs );

    largest_func_num = regs[ 0 ];

    if ( largest_func_num >= 0x1 ) {
        cpuid( 0x1, regs );

        family = ( regs[ 0 ] >> 8 ) & 0xF;
        model = ( regs[ 0 ] >> 4 ) & 0xF;

        if ( regs[ 3 ] & ( 1 << 4 ) ) {
            features |= CPU_FEATURE_TSC;
        }

        if ( regs[ 3 ] & ( 1 << 9 ) ) {
            features |= CPU_FEATURE_APIC;
        }

        if ( regs[ 3 ] & ( 1 << 11 ) ) {
            /* TODO: check processor signature */
            features |= CPU_FEATURE_SYSCALL;
        }

        if ( regs[ 3 ] & ( 1 << 12 ) ) {
            features |= CPU_FEATURE_MTRR;
        }

        if ( regs[ 3 ] & ( 1 << 23 ) ) {
            features |= CPU_FEATURE_MMX;
        }

        if ( regs[ 3 ] & ( 1 << 25 ) ) {
            features |= CPU_FEATURE_SSE;
        }
    }

    if ( largest_func_num >= 0x2 ) {
        cpuid( 0x2, regs );

        /* TODO: cache size detection */
    }

    /* Check extended functions */

    cpuid( 0x80000000, regs );

    largest_func_num = regs[ 0 ];

    if ( largest_func_num >= 0x80000001 ) {
        cpuid( 0x80000001, regs );

        if ( regs[ 3 ] & ( 1 << 11 ) ) {
            features |= CPU_FEATURE_SYSCALL;
        }
    }

    if ( largest_func_num >= 0x80000004 ) {
        cpuid( 0x80000002, ( register_t* )&name[ 0 ] );
        cpuid( 0x80000003, ( register_t* )&name[ 16 ] );
        cpuid( 0x80000004, ( register_t* )&name[ 32 ] );
    }

    /* Put the boot processor information to the screen */

    kprintf( "Boot processor: %s\n", name );
    kprintf( "Family: 0x%x Model: 0x%x\n", family, model );
    kprintf( "Features:" );

    for ( i = 0; i386_features[ i ].feature != 0; i++ ) {
        if ( ( features & i386_features[ i ].feature ) != 0 ) {
            kprintf( " %s", i386_features[ i ].name );
        }
    }

    kprintf( "\n" );

    /* TODO: Later move this to the SMP code */

    __asm__ __volatile__(
        "ltr %%ax\n"
        :
        : "a" ( GDT_ENTRIES * 8 )
    );

    for ( i = 0; i < MAX_CPU_COUNT; i++ ) {
        memcpy( processor_table[ i ].name, name, sizeof( name ) );
        processor_table[ i ].arch_data = ( void* )&arch_processor_table[ i ];

        arch_processor_table[ i ].family = family;
        arch_processor_table[ i ].model = model;
        arch_processor_table[ i ].features = features;
    }

    return 0;
}
