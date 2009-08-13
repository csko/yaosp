/* i386 specific SMP functions
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

#include <config.h>

#ifdef ENABLE_SMP

#include <smp.h>
#include <console.h>
#include <thread.h>
#include <macros.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/apic.h>
#include <arch/gdt.h>
#include <arch/idt.h>
#include <arch/interrupt.h>
#include <arch/mm/context.h>

atomic_t ap_running;
volatile uint32_t ap_stack_top;

uint32_t active_cpu_mask = 0;
volatile uint32_t tlb_invalidate_mask;

extern int __smp_trampoline_start;
extern int __smp_trampoline_end;

void processor_activated( void ) {
    active_cpu_mask |= ( 1 << get_processor_index() );
}

void flush_tlb_global( void ) {
    /* Flush the TLB on the current CPU */

    flush_tlb();

    /* If we have more than one active CPU in the system we have to send
       an IPI to others to flush the TLB on them as well. */

    if ( atomic_get( &active_processor_count ) > 1 ) {
        bool ints;
        uint32_t tmp;
        uint32_t timeout;

        /* Disable interrupts */

        ints = disable_interrupts();

        /* This will tell other CPUs that they have to invalidate their TLB */

        tlb_invalidate_mask = active_cpu_mask & ~( 1 << get_processor_index() );

        /* Send the IPI to other processors */

        /* Wait for busy to clear */

        while ( apic_read( LAPIC_ICR_LOW ) & ( 1 << 12 ) ) ;

        /* Build the command */

        tmp = apic_read( LAPIC_ICR_LOW );
        tmp &= ~0x000CDFFF;
        tmp |= APIC_TLB_FLUSH_IRQ; /* vector number */
        /* delivery mode: fixed */
        tmp |= ( 1 << 11 ); /* destination mode: logical */
        tmp |= ( 3 << 18 ); /* destination shorthand: all excluding self */

        /* Write the command to the APIC register */

        apic_write( LAPIC_ICR_LOW, tmp );

        /* Wait until other CPUs do the TLB flush */

        timeout = 50000000;

        while ( ( tlb_invalidate_mask != 0 ) && ( timeout > 0 ) ) {
            timeout--;
        }

        /* Re-enable interrupts if we disabled them */

        if ( ints ) {
            enable_interrupts();
        }

        /* Maybe a timeout happened? :( */

        if ( timeout == 0 ) {
            kprintf( WARNING, "Global TLB flush timed out!\n" );
        }
    }
}

void ap_processor_entry( void ) {
    gdt_t gdtp;
    idt_t idtp;
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )kernel_memory_context.arch_data;

    /* Enable paging */

    __asm__ __volatile__(
        "movl %0, %%cr3\n"
        "movl %%cr0, %0\n"
        "orl $0x80000000, %0\n"
        "movl %0, %%cr0\n"
        :
        : "r" ( arch_context->page_directory )
    );

    /* Load GDT */

    gdtp.size = sizeof( gdt ) - 1;
    gdtp.base = ( uint32_t )&gdt;

    __asm__ __volatile__(
        "lgdt %0\n"
        :
        : "m" ( gdtp )
    );

    reload_segment_descriptors();

    /* Load IDT */

    idtp.limit = ( sizeof( idt_descriptor_t ) * IDT_ENTRIES ) - 1;
    idtp.base = ( uint32_t )&idt;

    __asm__ __volatile__(
        "lidt %0\n"
        :
        : "m" ( idtp )
    );

    /* Setup our own TSS on this CPU */

    __asm__ __volatile__(
        "ltr %%ax\n"
        :
        : "a" ( ( GDT_ENTRIES + get_processor_index() ) * 8 )
    );

    /* Setup the local APIC and initialize the APIC timer */

    setup_local_apic();
    init_apic_timer();

    /* Make the new CPU active */

    atomic_inc( &active_processor_count );
    processor_activated();
    get_processor()->running = true;

    /* Just enable interrupts here and wait for the first timer interrupt,
       that will start the scheduler on this CPU as well. */

    enable_interrupts();
    halt_loop();
}

int arch_boot_processors( void ) {
    int i;
    size_t trampoline_size;

    trampoline_size = ( size_t )&__smp_trampoline_end - ( size_t )&__smp_trampoline_start;

    /* Copy the trampoline code to a known memory location below 1Mb */

    memcpy( ( void* )0x7000, ( void* )&__smp_trampoline_start, trampoline_size );

    for ( i = 1; i < processor_count; i++ ) {
        int j;

        /* Allocate a new stack for the AP */

        ap_stack_top = ( uint32_t )alloc_pages( 2, MEM_COMMON );
        ap_stack_top += 2 * PAGE_SIZE;
        ap_stack_top -= sizeof( register_t );
        atomic_set( &ap_running, 0 );

        DEBUG_LOG( "Booting CPU %d ...\n", i, ap_stack_top );

        /* Send INIT to the AP */

        apic_write( LAPIC_ICR_HIGH, arch_processor_table[ i ].apic_id << 24 );
        apic_write(
            LAPIC_ICR_LOW,
            ( 0x5 << 8 ) /* INIT */
        );

        /* Wait until it is delivered */

        while ( apic_read( LAPIC_ICR_LOW ) & ( 1 << 12 ) ) ;

        /* Give 10ms to the AP to initialize itself */

        thread_sleep( 10000 );

        int times_to_wait_for_ap[] = {
            10000, /* 10ms */
            50000, /* 50ms */
            100000, /* 100ms */
        };

        for ( j = 0; j < ARRAY_SIZE( times_to_wait_for_ap ) && !atomic_get( &ap_running ); j++ ) {
            uint32_t tmp;

            /* Send SIPI to the AP */

            tmp = apic_read( LAPIC_ICR_HIGH );
            tmp &= 0x00FFFFFF;
            tmp |= ( arch_processor_table[ i ].apic_id << 24 );
            apic_write( LAPIC_ICR_HIGH, tmp );

            apic_write(
                LAPIC_ICR_LOW,
                7 | /* vector */
                ( 0x6 << 8 ) /* SIPI */
            );

            /* Wait until it is delivered */

            while ( apic_read( LAPIC_ICR_LOW ) & ( 1 << 12 ) ) ;

            /* Let the AP start the boot process */

            thread_sleep( times_to_wait_for_ap[ j ] );
        }

        /* If the AP is started we wait until it finishes the kernel
           initialization, otherwise just print an error message and
           start booting the next AP. */

        if ( atomic_get( &ap_running ) ) {
            int tries = 0;

            do {
                thread_sleep( 10000 );
            } while ( ( !processor_table[ i ].running ) && ( tries++ < 500 ) );

            if ( processor_table[ i ].running ) {
                DEBUG_LOG( "CPU %d is running!\n", i );
            } else {
                kprintf( ERROR, "CPU %d started but failed to finish booting!\n", i );
            }
        } else {
            kprintf( ERROR, "Failed to start up CPU %d\n", i );
        }
    }

    return 0;
}

#endif /* ENABLE_SMP */
