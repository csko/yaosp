/* C entry point of the i386 architecture
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

#include <console.h>
#include <multiboot.h>
#include <mm/pages.h>
#include <mm/kmalloc.h>

#include <arch/screen.h>
#include <arch/gdt.h>
#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/mm/config.h>
#include <arch/mm/paging.h>

void arch_start( multiboot_header_t* header ) {
    int error;

    /* Initialize the screen */

    init_screen();
    kprintf( "Screen initialized.\n" );

    /* Setup our own Global Descriptor Table */

    kprintf( "Initializing GDT ... " );
    init_gdt();
    kprintf( "done\n" );

    /* Initialize CPU features */

    error = detect_cpu();

    if ( error < 0 ) {
        kprintf( "Failed to detect CPU: %d\n", error );
        return;
    }

    /* Initialize interrupts */

    kprintf( "Initializing interrupts ... " );
    init_interrupts();
    kprintf( "done\n" );

    /* Initialize page allocator */

    kprintf( "Initializing page allocator ... " );
    init_page_allocator( header );
    kprintf( "done\n" );
    kprintf( "Free memory: %u Kb\n", get_free_page_count() * PAGE_SIZE / 1024 );

    /* Initialize kmalloc */

    kprintf( "Initializing kmalloc ... " );

    error = init_kmalloc();

    if ( error < 0 ) {
        kprintf( "failed (error=%d)\n", error );
        return;
    }

    kprintf( "done\n" );

    /* Initialize paging */

    kprintf( "Initializing paging ... " );

    error = init_paging();

    if ( error < 0 ) {
        kprintf( "failed (error=%d)\n", error );
        return;
    }

    kprintf( "done\n" );
}
