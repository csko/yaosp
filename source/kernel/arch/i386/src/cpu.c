/* Processor detection code
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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
#include <kernel.h>
#include <lib/string.h>
#include <lib/ctype.h>

#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/pit.h>
#include <arch/io.h>

i386_cpu_t arch_processor_table[ MAX_CPU_COUNT ];

i386_feature_t i386_features[] = {
    { CPU_FEATURE_MMX, "mmx" },
    { CPU_FEATURE_SSE, "sse" },
    { CPU_FEATURE_APIC, "apic" },
    { CPU_FEATURE_MTRR, "mtrr" },
    { CPU_FEATURE_SYSCALL, "syscall" },
    { CPU_FEATURE_TSC, "tsc" },
    { CPU_FEATURE_SSE2, "sse2" },
    { CPU_FEATURE_HTT, "htt" },
    { CPU_FEATURE_SSE3, "sse3" },
    { CPU_FEATURE_PAE, "pae" },
    { CPU_FEATURE_IA64, "ia64" },
    { CPU_FEATURE_EST, "est" },
    { 0, "" }
};

extern uint32_t _kernel_stack_top;

static inline void cpuid( uint32_t reg, register_t* data ) {
    __asm__ __volatile__(
        "cpuid\n"
        : "=a" ( data[ 0 ] ), "=b" ( data[ 1 ] ), "=c" ( data[ 2 ] ), "=d" ( data[ 3 ] )
        : "0" ( reg )
    );
}

__init static bool cpuid_supported( void ) {
    register_t old_eflags;
    register_t new_eflags;

    old_eflags = get_eflags();
    set_eflags( old_eflags ^ EFLAG_ID );
    new_eflags = get_eflags();

    return ( ( old_eflags & EFLAG_ID ) != ( new_eflags & EFLAG_ID ) );
}

__init int detect_cpu( void ) {
    int i;
    int family = 0;
    int model = 0;
    int features = 0;
    register_t regs[ 4 ];
    register_t largest_func_num;
    char name[ 49 ] = { 0 };

    /* Clear the processor structures */

    memset( processor_table, 0, sizeof( cpu_t ) * MAX_CPU_COUNT );
    memset( arch_processor_table, 0, sizeof( i386_cpu_t ) * MAX_CPU_COUNT );

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

        if ( regs[ 3 ] & ( 1 << 6 ) ) {
            features |= CPU_FEATURE_PAE;
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

        if ( regs[ 3 ] & ( 1 << 26 ) ) {
            features |= CPU_FEATURE_SSE2;
        }

        if ( regs[ 3 ] & ( 1 << 28 ) ) {
            /* TODO: check ebx[23:16]>1 */
            features |= CPU_FEATURE_HTT;
        }

        if ( regs[ 3 ] & ( 1 << 30 ) ) {
            features |= CPU_FEATURE_IA64;
        }

        if ( regs[ 2 ] & ( 1 ) ) {
            features |= CPU_FEATURE_SSE3;
        }

        if ( regs[ 2 ] & ( 1 << 7) ) {
            features |= CPU_FEATURE_EST;
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
        size_t j;

        cpuid( 0x80000002, ( register_t* )&name[ 0 ] );
        cpuid( 0x80000003, ( register_t* )&name[ 16 ] );
        cpuid( 0x80000004, ( register_t* )&name[ 32 ] );

        for ( j = 0; j < 48; j++ ) {
            if ( !isspace( name[ j ] ) ) {
                break;
            }
        }

        /* Some CPU names have whitespace characters at the beginning, let's strip them */

        if ( j > 0 ) {
            size_t length;

            length = strlen( &name[ j ] );

            memmove( &name[ 0 ], &name[ j ], length );

            name[ length ] = 0;
        }
    }

    /* Put the boot processor information to the screen */

    kprintf( INFO, "Boot processor: %s\n", name );
    kprintf( INFO, "Features:" );

    for ( i = 0; i386_features[ i ].feature != 0; i++ ) {
        if ( ( features & i386_features[ i ].feature ) != 0 ) {
            kprintf( INFO, " %s", i386_features[ i ].name );
        }
    }

    kprintf( INFO, "\n" );

    /* Setup TSS for all possible CPU */

    for ( i = 0; i < MAX_CPU_COUNT; i++ ) {
        tss_t* tss = &arch_processor_table[ i ].tss;

        memset( tss, 0, sizeof( tss_t ) );

        tss->cs = KERNEL_CS;
        tss->ds = KERNEL_DS;
        tss->es = KERNEL_DS;
        tss->fs = KERNEL_DS;
        tss->eflags = 0x203246;
        tss->ss0 = KERNEL_DS;
        tss->esp0 = ( register_t )&_kernel_stack_top;
        tss->io_bitmap = 104;

        gdt_set_descriptor_base( ( GDT_ENTRIES + i ) * 8, ( uint32_t )tss );
        gdt_set_descriptor_limit( ( GDT_ENTRIES + i ) * 8, sizeof( tss_t ) );
        gdt_set_descriptor_access( ( GDT_ENTRIES + i ) * 8, 0x89 );
    }

    /* Load the TR register on the boot processor */

    __asm__ __volatile__(
        "ltr %%ax\n"
        :
        : "a" ( GDT_ENTRIES * 8 )
    );

    for ( i = 0; i < MAX_CPU_COUNT; i++ ) {
        strncpy( processor_table[ i ].name, name, MAX_PROCESSOR_NAME_LENGTH );
        processor_table[ i ].name[ MAX_PROCESSOR_NAME_LENGTH - 1 ] = 0;
        processor_table[ i ].features = features;

        processor_table[ i ].arch_data = ( void* )&arch_processor_table[ i ];

        arch_processor_table[ i ].family = family;
        arch_processor_table[ i ].model = model;
    }

    return 0;
}

__init int cpu_calibrate_speed( void ) {
    uint64_t start;
    uint64_t end;
    cpu_t* processor;

    processor = get_processor();

    outb( 0x34, PIT_MODE );
    outb( 0xFF, PIT_CH0 );
    outb( 0xFF, PIT_CH0 );

    pit_wait_wrap();
    start = rdtsc();
    pit_wait_wrap();
    end = rdtsc();

    processor->core_speed = ( uint64_t )PIT_TICKS_PER_SEC * ( end - start ) / 0xFFFF;

    kprintf( INFO, "CPU %d runs at %u MHz.\n", get_processor_index(), ( uint32_t )( processor->core_speed / 1000000 ) );

    outb( 0x34, PIT_MODE );
    outb( 0x00, PIT_CH0 );
    outb( 0x00, PIT_CH0 );

    return 0;
}
