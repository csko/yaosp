/* Processor detection code
 *
 * Copyright (c) 2008, 2009, 2010 Zoltan Kovacs
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
#include <macros.h>
#include <time.h>
#include <lib/string.h>
#include <lib/ctype.h>
#include <lib/random.h>

#include <arch/cpu.h>
#include <arch/gdt.h>
#include <arch/pit.h>
#include <arch/io.h>
#include <arch/acpi.h>
#include <arch/hpet.h>

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

uint64_t tsc_to_us;

uint32_t cpu_khz;
uint32_t tsc_khz;
uint64_t tsc_to_ns_scale;

i386_cpu_t arch_processor_table[ MAX_CPU_COUNT ];

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
    for (i = 0; i < MAX_CPU_COUNT; i++) {
        int entry;
        tss_t* tss = &arch_processor_table[i].tss;

        memset(tss, 0, sizeof(tss_t) );

        tss->cs = KERNEL_CS;
        tss->ds = KERNEL_DS;
        tss->es = KERNEL_DS;
        tss->fs = KERNEL_DS;
        tss->eflags = 0x203246;
        tss->ss0 = KERNEL_DS;
        tss->esp0 = ( register_t )&_kernel_stack_top;
        tss->io_bitmap = 104;

        entry = (GDT_ENTRIES + i) * 8;
        gdt_set_descriptor_base(entry, (uint32_t)tss);
        gdt_set_descriptor_limit(entry, sizeof(tss_t));
        gdt_set_descriptor_access(entry, 0x89);
    }

    /* Setup GS segment for all possible CPU */
    for (i = 0; i < MAX_CPU_COUNT; i++) {
        int entry;

        entry = (GDT_ENTRIES + MAX_CPU_COUNT + i) * 8;
        gdt_set_descriptor_base(entry, 0);
        gdt_set_descriptor_limit(entry, TLD_SIZE * sizeof(ptr_t));
        gdt_set_descriptor_access(entry, 0xf2);
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

    /* Initialize random number generator */
    random_init(rdtsc());

    return 0;
}

static inline int pit_verify_msb( uint8_t val ) {
    inb( 0x42 ); /* ignore LSB */

    return ( inb( 0x42 ) == val );
}

static inline int pit_expect_msb( uint8_t val, uint64_t* tscp, unsigned long* deltap ) {
    int count;
    uint64_t tsc = 0;

    for ( count = 0; count < 50000; count++ ) {
        if ( !pit_verify_msb( val ) ) {
            break;
        }

        tsc = rdtsc();
    }

    *deltap = rdtsc() - tsc;
    *tscp = tsc;

    return ( count > 5 );
}

static inline uint32_t pit_wait_start( void ) {
    uint8_t low;
    uint8_t high;

    do {
        outb( 0x00, PIT_MODE );
        low = inb( PIT_CH0 );
        high = inb( PIT_CH0 );
    } while ( high != 255 );

    return ( ( ( uint32_t )high << 8 ) | low );
}

static inline uint32_t pit_wait_until( uint8_t wait_high ) {
    uint8_t low;
    uint8_t high;

    do {
        outb( 0x00, PIT_MODE );
        low = inb( PIT_CH0 );
        high = inb( PIT_CH0 );
    } while ( high > wait_high );

    return ( ( ( uint32_t )high << 8 ) | low );
}

static uint32_t slow_pit_calibrate( void ) {
    double r1, r2, r3;
    uint32_t pit_before, pit_after;
    uint64_t tsc_before, tsc_after;

    outb( 0x34, PIT_MODE );
    outb( 0xFF, PIT_CH0 );
    outb( 0xFF, PIT_CH0 );

 calibrate1:
    pit_before = pit_wait_start();
    tsc_before = rdtsc();

    pit_after = pit_wait_until( 224 );
    tsc_after = rdtsc();

    r1 = ( double )( tsc_after - tsc_before ) / ( double )( pit_before - pit_after );

 calibrate2:
    pit_before = pit_wait_start();
    tsc_before = rdtsc();

    pit_after = pit_wait_until( 192 );
    tsc_after = rdtsc();

    r2 = ( double )( tsc_after - tsc_before ) / ( double )( pit_before - pit_after );

    if ( ( ( r1 / r2 ) < 0.99 ) ||
         ( ( r1 / r2 ) > 1.01 ) ) {
        goto calibrate1;
    }

    pit_before = pit_wait_start();
    tsc_before = rdtsc();

    pit_after = pit_wait_until( 128 );
    tsc_after = rdtsc();

    r3 = ( double )( tsc_after - tsc_before ) / ( double )( pit_before - pit_after );

    if ( ( ( r2 / r3 ) < 0.99 ) ||
         ( ( r2 / r3 ) > 1.01 ) ) {
        goto calibrate2;
    }

    return ( tsc_after - tsc_before ) * PIT_TICKS_PER_SEC / ( pit_before - pit_after ) / 1000;
}

#define MAX_QUICK_PIT_MS 25
#define MAX_QUICK_PIT_ITERATIONS (MAX_QUICK_PIT_MS * PIT_TICKS_PER_SEC / 1000 / 256)

static uint32_t quick_pit_calibrate( void ) {
    int i;
    uint64_t tsc, delta;
    unsigned long d1, d2;

    /* Set the Gate high, disable speaker */
    outb((inb(0x61) & ~0x02) | 0x01, 0x61);

    /*
     * Counter 2, mode 0 (one-shot), binary count
     *
     * NOTE! Mode 2 decrements by two (and then the
     * output is flipped each time, giving the same
     * final output frequency as a decrement-by-one),
     * so mode 0 is much better when looking at the
     * individual counts.
     */
    outb( 0xb0, 0x43 );

    /* Start at 0xffff */
    outb( 0xff, 0x42 );
    outb( 0xff, 0x42 );

    /*
     * The PIT starts counting at the next edge, so we
     * need to delay for a microsecond. The easiest way
     * to do that is to just read back the 16-bit counter
     * once from the PIT.
     */
    pit_verify_msb( 0 );

    if ( pit_expect_msb( 0xff, &tsc, &d1 ) ) {
        for ( i = 1; i <= MAX_QUICK_PIT_ITERATIONS; i++ ) {
            if ( !pit_expect_msb( 0xff - i, &delta, &d2 ) ) {
                break;
            }

            /*
             * Iterate until the error is less than 500 ppm
             */
            delta -= tsc;

            if ( ( d1 + d2 ) >= ( delta >> 11 ) ) {
                continue;
            }

            /*
             * Check the PIT one more time to verify that
             * all TSC reads were stable wrt the PIT.
             *
             * This also guarantees serialization of the
             * last cycle read ('d2') in pit_expect_msb.
             */
            if ( !pit_verify_msb( 0xfe - i ) ) {
                break;
            }

            goto success;
        }
    }

    kprintf( INFO, "Fast TSC calibration failed.\n" );

    return 0;

success:
    /*
     * Ok, if we get here, then we've seen the
     * MSB of the PIT decrement 'i' times, and the
     * error has shrunk to less than 500 ppm.
     *
     * As a result, we can depend on there not being
     * any odd delays anywhere, and the TSC reads are
     * reliable (within the error). We also adjust the
     * delta to the middle of the error bars, just
     * because it looks nicer.
     *
     * kHz = ticks / time-in-seconds / 1000;
     * kHz = (t2 - t1) / (I * 256 / PIT_TICKS_PER_SEC) / 1000
     * kHz = ((t2 - t1) * PIT_TICKS_PER_SEC) / (I * 256 * 1000)
     */

    delta += ( long )( d2 - d1 ) / 2;
    delta *= PIT_TICKS_PER_SEC;
    delta /= ( i * 256 * 1000 );

    kprintf( INFO, "Fast TSC calibration using PIT.\n" );

    return delta;
}

#define CAL_MS      10
#define CAL_LATCH   (PIT_TICKS_PER_SEC / (1000 / CAL_MS))
#define CAL_PIT_LOOPS   1000

#define CAL2_MS     50
#define CAL2_LATCH  (PIT_TICKS_PER_SEC / (1000 / CAL2_MS))
#define CAL2_PIT_LOOPS  5000

#define LONG_MAX        ((__LONG_MAX__) + 0L)
#define ULONG_MAX       (LONG_MAX * 2UL + 1UL)
#define ULLONG_MAX 18446744073709551615ULL

#define MAX_RETRIES     5
#define SMI_TRESHOLD    50000

static unsigned long pit_calibrate_tsc( uint32_t latch, unsigned long ms, int loopmin ) {
    uint64_t tsc, t1, t2, delta;
    unsigned long tscmin, tscmax;
    int pitcnt;

    /* Set the Gate high, disable speaker */
    outb( ( inb( 0x61 ) & ~0x02 ) | 0x01, 0x61 );

    /*
     * Setup CTC channel 2* for mode 0, (interrupt on terminal
     * count mode), binary count. Set the latch register to 50ms
     * (LSB then MSB) to begin countdown.
     */
    outb( 0xb0, 0x43 );
    outb( latch & 0xff, 0x42 );
    outb( latch >> 8, 0x42 );

    tsc = t1 = t2 = rdtsc();

    pitcnt = 0;
    tscmax = 0;
    tscmin = ULONG_MAX;

    while ( ( inb( 0x61 ) & 0x20 ) == 0 ) {
        t2 = rdtsc();
        delta = t2 - tsc;
        tsc = t2;

        if ((unsigned long) delta < tscmin)
            tscmin = (unsigned int) delta;

        if ((unsigned long) delta > tscmax)
            tscmax = (unsigned int) delta;

        pitcnt++;
    }

    /*
     * Sanity checks:
     *
     * If we were not able to read the PIT more than loopmin
     * times, then we have been hit by a massive SMI
     *
     * If the maximum is 10 times larger than the minimum,
     * then we got hit by an SMI as well.
     */
    if ( ( pitcnt < loopmin ) ||
         ( tscmax > 10 * tscmin ) ) {
        return ULONG_MAX;
    }

    /* Calculate the PIT value */
    delta = ( t2 - t1 ) / ms;

    return delta;
}

static uint64_t tsc_read_refs( uint64_t* p, int hpet ) {
    int i;
    uint64_t t1, t2;

    for ( i = 0; i < MAX_RETRIES; i++ ) {
        t1 = rdtsc();

        if ( hpet ) {
            *p = hpet_readl( HPET_COUNTER ) & 0xFFFFFFFF;
        } else {
            *p = acpi_pmtimer_read();
        }

        t2 = rdtsc();

        if ( ( t2 - t1 ) < SMI_TRESHOLD ) {
            return t2;
        }
    }

    return ULLONG_MAX;
}

static uint32_t calc_hpet_ref( uint64_t deltatsc, uint64_t hpet1, uint64_t hpet2 ) {
    uint64_t tmp;

    if ( hpet2 < hpet1 ) {
        hpet2 += 0x100000000ULL;
    }

    hpet2 -= hpet1;

    tmp = ( ( uint64_t )hpet2 * hpet_readl( HPET_PERIOD ) );
    tmp /= 1000000;
    deltatsc /= tmp;

    return ( uint32_t )deltatsc;
}

static uint32_t calc_pmtimer_ref( uint64_t deltatsc, uint64_t pm1, uint64_t pm2 ) {
    uint64_t tmp;

    if ( !pm1 && !pm2 ) {
        return ULONG_MAX;
    }

    if ( pm2 < pm1 ) {
        pm2 += ( uint64_t )ACPI_PM_OVRRUN;
    }

    pm2 -= pm1;
    tmp = pm2 * 1000000000LL;

    tmp /= PMTMR_TICKS_PER_SEC;
    deltatsc /= tmp;

    return ( uint32_t )deltatsc;
}

static uint32_t tsc_calibrate( void ) {
    int i, loopmin;
    uint32_t tsc_pit_min, tsc_ref_min;
    uint32_t latch, ms, fast_calibrate;
    uint64_t tsc1, tsc2, delta, ref1, ref2;

    fast_calibrate = quick_pit_calibrate();

    if ( fast_calibrate != 0 ) {
        return fast_calibrate;
    }

    /* Preset PIT loop values */

    ms = CAL_MS;
    latch = CAL_LATCH;
    loopmin = CAL_PIT_LOOPS;

    tsc_pit_min = ULONG_MAX;
    tsc_ref_min = ULONG_MAX;

    for ( i = 0; i < 3; i++ ) {
        unsigned long tsc_pit_khz;

        /*
         * Read the start value and the reference count of
         * hpet/pmtimer when available. Then do the PIT
         * calibration, which will take at least 50ms, and
         * read the end value.
         */
        tsc1 = tsc_read_refs( &ref1, hpet_present );
        tsc_pit_khz = pit_calibrate_tsc( latch, ms, loopmin );
        tsc2 = tsc_read_refs( &ref2, hpet_present );

        /* Pick the lowest PIT TSC calibration so far */
        tsc_pit_min = MIN( tsc_pit_min, tsc_pit_khz );

        /* hpet or pmtimer available? */
        if ( !hpet_present && !ref1 && !ref2 ) {
            continue;
        }

        /* Check, whether the sampling was disturbed by an SMI */
        if ( ( tsc1 == ULLONG_MAX ) ||
             ( tsc2 == ULLONG_MAX ) ) {
            continue;
        }

        tsc2 = ( tsc2 - tsc1 ) * 1000000LL;

        if ( hpet_present ) {
            tsc2 = calc_hpet_ref( tsc2, ref1, ref2 );
        } else {
            tsc2 = calc_pmtimer_ref( tsc2, ref1, ref2 );
        }

        tsc_ref_min = MIN( tsc_ref_min, ( uint32_t )tsc2 );

        /* Check the reference deviation */

        delta = ( ( uint64_t )tsc_pit_min ) * 100;
        delta /= tsc_ref_min;

        /*
         * If both calibration results are inside a 10% window
         * then we can be sure, that the calibration
         * succeeded. We break out of the loop right away. We
         * use the reference value, as it is more precise.
         */
        if ( ( delta >= 90 ) &&
             ( delta <= 110 ) ) {
            kprintf(
                INFO, "TSC: PIT calibration matches %s (%d loops).\n",
                hpet_present ? "HPET" : "PMTIMER", i + 1
            );

            return tsc_ref_min;
        }

        /*
         * Check whether PIT failed more than once. This
         * happens in virtualized environments. We need to
         * give the virtual PC a slightly longer timeframe for
         * the HPET/PMTIMER to make the result precise.
         */
        if ( ( i == 1 ) &&
             ( tsc_pit_min == ULONG_MAX ) ) {
            ms = CAL2_MS;
            latch = CAL2_LATCH;
            loopmin = CAL2_PIT_LOOPS;
        }
    }

    /*
     * Now check the results.
     */
    if ( tsc_pit_min == ULONG_MAX ) {
        /* PIT gave no useful value */
        kprintf( INFO, "TSC: Unable to calibrate against PIT.\n" );

        /* We don't have an alternative source, disable TSC */
        if ( !hpet_present && !ref1 && !ref2 ) {
            kprintf( INFO, "TSC: No reference (HPET/PMTIMER) available.\n" );
            return slow_pit_calibrate();
        }

        /* The alternative source failed as well, disable TSC */
        if ( tsc_ref_min == ULONG_MAX ) {
            kprintf( INFO, "TSC: HPET/PMTIMER calibration failed.\n" );
            return slow_pit_calibrate();
        }

        /* Use the alternative source */
        kprintf(
            INFO, "TSC: using %s reference calibration.\n",
            hpet_present ? "HPET" : "PMTIMER"
        );

        return tsc_ref_min;
    }

    /* We don't have an alternative source, use the PIT calibration value */
    if ( !ref1 && !ref2 ) {
        kprintf( INFO, "TSC: Using PIT calibration value.\n" );
        return tsc_pit_min;
    }

    /* The alternative source failed, use the PIT calibration value */
    if ( tsc_ref_min == ULONG_MAX ) {
        kprintf( INFO, "TSC: HPET/PMTIMER calibration failed. Using PIT calibration.\n" );
        return tsc_pit_min;
    }

    /*
     * The calibration values differ too much. In doubt, we use
     * the PIT value as we know that there are PMTIMERs around
     * running at double speed. At least we let the user know:
     */
    kprintf(
        INFO, "TSC: PIT calibration deviates from %s: %lu %lu.\n",
        hpet_present ? "HPET" : "PMTIMER", tsc_pit_min, tsc_ref_min
    );
    kprintf( INFO, "TSC: Using PIT calibration value.\n" );

    return tsc_pit_min;
}

__init int cpu_calibrate_speed( void ) {
    int i;

    /* Calibrate the TSC and CPU frequency. */

    tsc_khz = tsc_calibrate();
    cpu_khz = tsc_khz;

    kprintf(
        INFO, "Detected %u.%u MHz processor.\n",
        cpu_khz / 1000, cpu_khz % 1000
    );

    tsc_to_ns_scale = ( NSEC_PER_MSEC << CYC2NS_SCALE_FACTOR ) / cpu_khz;

    /* Update the CPU table according to the detected speed. */

    for ( i = 0; i < MAX_CPU_COUNT; i++ ) {
        processor_table[ i ].core_speed = cpu_khz * 1000;
    }

    return 0;
}
