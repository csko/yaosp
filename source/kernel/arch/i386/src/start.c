/* C entry point of the i386 architecture
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

#include <console.h>
#include <multiboot.h>
#include <kernel.h>
#include <bootmodule.h>
#include <version.h>
#include <macros.h>
#include <linker/elf32.h>
#include <mm/pages.h>
#include <mm/kmalloc.h>
#include <mm/region.h>
#include <lib/string.h>

#include <arch/screen.h>
#include <arch/gdt.h>
#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/pit.h>
#include <arch/io.h>
#include <arch/mp.h>
#include <arch/apic.h>
#include <arch/bios.h>
#include <arch/mm/config.h>
#include <arch/mm/paging.h>

extern int __kernel_end;

multiboot_header_t mb_header;

__init static int arch_init_page_allocator( multiboot_header_t* header ) {
    int error;
    int module_count;
    uint32_t mem_size;
    uint32_t mmap_length;
    ptr_t first_free_address;
    multiboot_mmap_entry_t* mmap_entry;

    /* Calculate the address of the first usable memory page */

    first_free_address = ( ptr_t )&__kernel_end;

    ASSERT( ( first_free_address % PAGE_SIZE ) == 0 );

    module_count = get_bootmodule_count();

    if ( module_count > 0 ) {
        int j;
        ptr_t module_end;
        bootmodule_t* module;

        for ( j = 0; j < module_count; j++ ) {
            module = get_bootmodule_at( j );
            module_end = PAGE_ALIGN( ( ptr_t )module->address + module->size );

            if ( module_end > first_free_address ) {
                first_free_address = module_end;
            }
        }
    }

    if ( header->flags & MB_FLAG_ELF_SYM_INFO ) {
        uint32_t i;
        elf_section_header_t* section_header;

        first_free_address = MAX( first_free_address, PAGE_ALIGN( header->elf_info.addr + header->elf_info.num * header->elf_info.size ) );

        for ( i = 1; i < header->elf_info.num; i++ ) {
            section_header = ( elf_section_header_t* )( header->elf_info.addr + i * header->elf_info.size );

            switch ( section_header->type ) {
                case SECTION_STRTAB :
                case SECTION_SYMTAB :
                    first_free_address = MAX( first_free_address, PAGE_ALIGN( section_header->address + section_header->size ) );
                    break;
            }
        }
    }

    /* Initialize the page allocator */

    mem_size = header->memory_upper * 1024 + 1024 * 1024;

    error = init_page_allocator( first_free_address, mem_size );

    if ( error < 0 ) {
        return error;
    }

    /* Register memory types */

    register_memory_type( MEM_COMMON, 0x100000, header->memory_upper * 1024 );
    register_memory_type( MEM_LOW, 0x0, 1024 * 1024 );

    init_page_allocator_late();

    /* Reserve the first page of the memory (realmode IVT) */

    reserve_memory_pages( 0x0, PAGE_SIZE );

    /* Reserve kernel memory pages */

    reserve_memory_pages( 0x100000, ( ptr_t )&__kernel_end - 0x100000 );

    /* Reserve bootmodule memory pages */

    if ( module_count > 0 ) {
        int j;
        bootmodule_t* module;

        for ( j = 0; j < module_count; j++ ) {
            module = get_bootmodule_at( j );

            reserve_memory_pages( ( ptr_t )module->address, PAGE_ALIGN( module->size ) );
        }
    }

    /* Reserve region containing ELF informations, if any */

    if ( header->flags & MB_FLAG_ELF_SYM_INFO ) {
        uint32_t i;
        elf_section_header_t* section_header;

        /* Section headers */

        reserve_memory_pages( ( ptr_t )header->elf_info.addr, PAGE_ALIGN( header->elf_info.num * header->elf_info.size ) );

        /* Required ELF sections */

        for ( i = 1; i < header->elf_info.num; i++ ) {
            section_header = ( elf_section_header_t* )( header->elf_info.addr + i * header->elf_info.size );

            switch ( section_header->type ) {
                case SECTION_STRTAB :
                case SECTION_SYMTAB :
                    reserve_memory_pages(
                        ( ptr_t )( section_header->address & PAGE_MASK ),
                        PAGE_ALIGN( section_header->size + ( section_header->address & ~PAGE_MASK ) )
                    );

                    break;
            }
        }
    }

    /* Reserve not usable memory pages reported by GRUB */

    mmap_length = 0;
    mmap_entry = ( multiboot_mmap_entry_t* )header->memory_map_address;

    for ( ; mmap_length < header->memory_map_length;
            mmap_length += ( mmap_entry->size + 4 ),
            mmap_entry = ( multiboot_mmap_entry_t* )( ( uint8_t* )mmap_entry + mmap_entry->size + 4 ) ) {
        if ( mmap_entry->type != 1 ) {
            uint64_t real_base;
            uint64_t real_length;

            real_base = mmap_entry->base & PAGE_MASK;
            real_length = PAGE_ALIGN( ( mmap_entry->base & ~PAGE_MASK ) + mmap_entry->length );

            /* Make sure the region is inside the available physical memory */

            if ( real_base >= mem_size ) {
                continue;
            }

            if ( real_length > ( mem_size - real_base ) ) {
                real_length = mem_size - real_base;
            }

            /* Reserve the region */

            reserve_memory_pages( real_base, real_length );
        }
    }

    return 0;
}

__init void arch_start( multiboot_header_t* header ) {
    int error;

    /* Save the multiboot structure */

    memcpy( &mb_header, header, sizeof( multiboot_header_t ) );

    /* Save the kernel parameters before we write to any memory location */

    parse_kernel_parameters( header->kernel_parameters );

    /* Initialize the screen */

    init_screen();

    kprintf(
        INFO,
        "Booting yaOSp %d.%d.%d built on %s %s.\n",
        KERNEL_MAJOR_VERSION,
        KERNEL_MINOR_VERSION,
        KERNEL_RELEASE_VERSION,
        build_date,
        build_time
    );

    /* Setup our own Global Descriptor Table */

    init_gdt();

    /* Initialize CPU features */

    error = detect_cpu();

    if ( error < 0 ) {
        kprintf( ERROR, "Failed to detect CPU: %d\n", error );
        return;
    }

    /* Initialize interrupts */

    init_interrupts();

    /* Calibrate the boot CPU speed */

    cpu_calibrate_speed();

    /* Initializing bootmodules */

    init_bootmodules( header );

    if ( get_bootmodule_count() > 0 ) {
        kprintf( INFO, "Loaded %d module(s)\n", get_bootmodule_count() );
    }

    /* Initialize page allocator */

    error = arch_init_page_allocator( header );

    if ( error < 0 ) {
        kprintf( ERROR, "Failed to initialize page allocator (error=%d)\n", error );
        return;
    }

    kprintf( INFO, "Free memory: %u Kb\n", get_free_page_count() * PAGE_SIZE / 1024 );

    /* Initialize kmalloc */

    error = init_kmalloc();

    if ( error < 0 ) {
        kprintf( ERROR, "Failed to initialize kernel memory allocator (error=%d)\n", error );
        return;
    }

    /* Initialize memory region manager */

    preinit_regions();

    /* Initialize paging */

    error = init_paging();

    if ( error < 0 ) {
        kprintf( ERROR, "Failed to initialize paging (error=%d)\n", error );
        return;
    }

    /* Call the architecture independent entry
       point of the kernel */

    kernel_main();
}

__init int arch_late_init( void ) {
    init_mp();
    init_apic();
    init_pit();
    init_apic_timer();
    init_system_time();
    init_elf32_kernel_symbols();
    init_elf32_module_loader();
    init_elf32_application_loader();
    init_bios_access();

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
    bios_regs_t regs;

    memset( &regs, 0, sizeof( bios_regs_t ) );

    regs.ax = 0x5304;
    regs.bx = 0;
    call_bios_interrupt( 0x15, &regs );

    regs.ax = 0x5302;
    regs.bx = 0;
    call_bios_interrupt( 0x15, &regs );

    regs.ax = 0x5308;
    regs.bx = 1;
    regs.cx = 1; /* enabled */
    call_bios_interrupt( 0x15, &regs );

    regs.ax = 0x530D;
    regs.bx = 1;
    regs.cx = 1; /* enable power management */
    call_bios_interrupt( 0x15, &regs );

    regs.ax = 0x530F;
    regs.bx = 1;
    regs.cx = 1; /* engage power management */
    call_bios_interrupt( 0x15, &regs );

    regs.ax = 0x530E;
    regs.bx = 0;
    regs.cx = 0x102;
    call_bios_interrupt( 0x15, &regs );

    regs.ax = 0x5307;
    regs.bx = 1;
    regs.cx = 3; /* turn off system */
    call_bios_interrupt( 0x15, &regs );

    disable_interrupts();
    halt_loop();
}
