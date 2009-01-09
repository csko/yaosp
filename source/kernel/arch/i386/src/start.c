/* C entry point of the i386 architecture
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#include <console.h>
#include <multiboot.h>
#include <kernel.h>
#include <bootmodule.h>
#include <mm/pages.h>
#include <mm/kmalloc.h>
#include <mm/region.h>

#include <arch/screen.h>
#include <arch/gdt.h>
#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/pit.h>
#include <arch/elf32.h>
#include <arch/io.h>
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

    /* Initializing bootmodules */

    kprintf( "Initializing bootmodules ... " );
    init_bootmodules( header );
    kprintf( "done\n" );

    if ( get_bootmodule_count() > 0 ) {
        kprintf( "Loaded %d module(s)\n", get_bootmodule_count() );
    }

    /* Initialize page allocator */

    kprintf( "Initializing page allocator ... " );

    error = init_page_allocator( header );

    if ( error < 0 ) {
        kprintf( "failed (error=%d)\n", error );
        return;
    }

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

    /* Initialize memory region manager */

    kprintf( "Initializing region manager ... " );
    preinit_regions();
    kprintf( "done\n" );

    /* Initialize paging */

    kprintf( "Initializing paging ... " );

    error = init_paging();

    if ( error < 0 ) {
        kprintf( "failed (error=%d)\n", error );
        return;
    }

    kprintf( "done\n" );

    /* Call the architecture independent entry
       point of the kernel */

    kernel_main();
}

int arch_late_init( void ) {
    init_pit();
    init_system_time();
    init_elf32_module_loader();
    init_elf32_application_loader();

    return 0;
}

void arch_reboot( void ) {
    int i;

    disable_interrupts();

    /* Flush keyboard */
    for ( ; ( ( i = inb( 0x64 ) ) & 0x01 ) != 0 && ( i & 0x02 ) != 0 ; ) ;

    /* CPU RESET */
    outb( 0xFE, 0x64 );

    halt_loop();
}

void arch_shutdown( void ) {
    kprintf( "Shutdown complete!\n" );
    disable_interrupts();
    halt_loop();
}
