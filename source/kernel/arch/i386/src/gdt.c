/* Global Descriptor Table handling
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

#include <lib/string.h>

#include <arch/gdt.h>
#include <arch/cpu.h>

extern uint32_t _kernel_stack_top;

gdt_descriptor_t gdt[ GDT_ENTRIES + MAX_CPU_COUNT ] = {
  /* NULL descriptor */
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* Kernel CS */
  { 0xFFFF, 0, 0, 0x9A, 0xF, 0, 0, 1, 1, 0 },
  /* Kernel DS */
  { 0xFFFF, 0, 0, 0x92, 0xF, 0, 0, 1, 1, 0 },
  /* User CS */
  { 0xFFFF, 0, 0, 0xFE, 0xF, 0, 0, 1, 1, 0 },
  /* User DS */
  { 0xFFFF, 0, 0, 0xF2, 0xF, 0, 0, 1, 1, 0 },
};

static void set_descriptor_base( uint16_t desc, uint32_t base ) {
    desc /= 8;

    gdt[ desc ].base_low = base & 0xFFFF;
    gdt[ desc ].base_mid = ( base >> 16 ) & 0xFF;
    gdt[ desc ].base_high = ( base >> 24 ) & 0xFF;
}

static void set_descriptor_limit( uint16_t desc, uint32_t limit ) {
    desc /= 8;

    gdt[ desc ].limit_low = limit & 0xFFFF;
    gdt[ desc ].limit_high = ( limit >> 16 ) & 0xF;
}

static void set_descriptor_access( uint16_t desc, uint8_t access ) {
    desc /= 8;

    gdt[ desc ].access = access;
}

int init_gdt( void ) {
    int i;
    gdt_t gdtp;

    memset( arch_processor_table, 0, sizeof( i386_cpu_t ) * MAX_CPU_COUNT );

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

        set_descriptor_base( ( GDT_ENTRIES + i ) * 8, ( uint32_t )tss );
        set_descriptor_limit( ( GDT_ENTRIES + i ) * 8, sizeof( tss_t ) );
        set_descriptor_access( ( GDT_ENTRIES + i ) * 8, 0x89 );
    }

    gdtp.size = sizeof( gdt ) - 1;
    gdtp.base = ( uint32_t )&gdt;

    __asm__ __volatile__(
        "lgdt %0\n"
        :
        : "m" ( gdtp )
    );

    reload_segment_descriptors();

    return 0;
}
