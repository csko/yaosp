/* Page fault handler
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

#include <types.h>
#include <console.h>
#include <smp.h>
#include <errno.h>
#include <semaphore.h>
#include <macros.h>
#include <scheduler.h>
#include <debug.h>
#include <signal.h>
#include <mm/region.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/mm/paging.h>

extern semaphore_id region_lock;

void dump_registers( registers_t* regs );

static void invalid_page_fault( thread_t* thread, registers_t* regs, uint32_t cr2, const char* message ) {
    kprintf( "Invalid page fault at 0x%x (%s)\n", cr2, message );
    dump_registers( regs );
    debug_print_stack_trace();

    if ( thread != NULL ) {
        kprintf( "Process: %s thread: %s\n", thread->process->name, thread->name );
        memory_context_dump( thread->process->memory_context );

        if ( regs->error_code & 0x4 ) {
            send_signal( thread, SIGSEGV );
        } else {
            thread_exit( SIGSEGV );
        }
    } else {
        disable_interrupts();
        halt_loop();
    }
}

static int handle_lazy_page_allocation( region_t* region, uint32_t address ) {
    void* p;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;
    memory_context_t* context;
    i386_memory_context_t* arch_context;

    context = region->context;
    arch_context = ( i386_memory_context_t* )context->arch_data;

    pgd_entry = page_directory_entry( arch_context, address );

    if ( *pgd_entry == 0 ) {
        p = alloc_pages( 1, MEM_COMMON );

        if ( p == NULL ) {
            return -ENOMEM;
        }

        memsetl( p, 0, PAGE_SIZE / 4 );

        *pgd_entry = ( uint32_t )p | PRESENT | WRITE | USER;
    }

    pt_entry = page_table_entry( *pgd_entry, address );

    p = alloc_pages( 1, MEM_COMMON );

    if ( p == NULL ) {
        return -ENOMEM;
    }

    memsetl( p, 0, PAGE_SIZE / 4 );

    *pt_entry = ( uint32_t )p | PRESENT | WRITE | USER;

    /* Invalidate the TLB because we just mapped a new page */

    /* TOOD: use invlpg instead of flush_tlb()! */
    flush_tlb();

    return 0;
}

#define MAX_PAGES_PER_LOAD 6

static int handle_lazy_page_loading( region_t* region, uint32_t address, uint32_t* pages_loaded ) {
    int i;
    void* p;
    int error;
    uint32_t* pt;
    uint32_t pt_index;
    uint32_t pgd_index;
    uint32_t page_count;
    uint32_t region_offset;
    memory_context_t* context;
    i386_memory_context_t* arch_context;

    context = region->context;
    arch_context = ( i386_memory_context_t* )context->arch_data;

    address &= PAGE_MASK;
    ASSERT( address >= region->start );

    region_offset = address - region->start;

    /* Calculate the number of pages that we can load */

    page_count = PAGE_ALIGN( region->size - region_offset ) / PAGE_SIZE;
    page_count = MIN( page_count, MAX_PAGES_PER_LOAD );

    /* Map the page tables */

    error = map_region_page_tables( arch_context, address, page_count * PAGE_SIZE, false );

    if ( error < 0 ) {
        return error;
    }

    /* Count the unmapped pages */

    pgd_index = address >> PGDIR_SHIFT;
    pt_index = ( address >> PAGE_SHIFT ) & 1023;
    pt = ( uint32_t* )( arch_context->page_directory[ pgd_index ] & PAGE_MASK );

    for ( i = 0; i < page_count; i++ ) {
        if ( pt[ pt_index ] != 0 ) {
            break;
        }

        if ( pt_index == 1023 ) {
            pgd_index++;
            pt_index = 0;

            pt = ( uint32_t* )( arch_context->page_directory[ pgd_index ] & PAGE_MASK );
        } else {
            pt_index++;
        }
    }

    ASSERT( i > 0 );

    page_count = i;

    /* Allocate memory for the new pages */

    p = alloc_pages( page_count, MEM_COMMON );

    if ( p == NULL ) {
        return -ENOMEM;
    }

    /* Load the data to the newly allocated pages */

    if ( region_offset >= region->file_size ) {
        memsetl( p, 0, page_count * PAGE_SIZE / 4 );
    } else {
        off_t file_read_offset;
        uint32_t file_data_size;

        file_read_offset = region->file_offset + region_offset;
        file_data_size = region->file_size - region_offset;
        file_data_size = MIN( file_data_size, page_count * PAGE_SIZE );

        ASSERT( file_data_size > 0 );
        ASSERT( file_data_size <= page_count * PAGE_SIZE );

        if ( do_pread_helper( region->file, p, file_data_size, file_read_offset ) != file_data_size ) {
            free_pages( p, page_count );
            return -EIO;
        }

        if ( file_data_size < page_count * PAGE_SIZE ) {
            memset( ( uint8_t* )p + file_data_size, 0, page_count * PAGE_SIZE - file_data_size );
        }
    }

    /* Map the new pages */

    error = map_region_pages(
        arch_context,
        address,
        ( ptr_t )p,
        page_count * PAGE_SIZE,
        false,
        ( region->flags & REGION_WRITE ) != 0
    );

    if ( error < 0 ) {
        return error;
    }

    /* Invalidate the TLB */

    flush_tlb();

    *pages_loaded = page_count;

    return 0;
}

int handle_page_fault( registers_t* regs ) {
    int error;
    uint32_t cr2;
    region_t* region;
    thread_t* thread;
    const char* message;

    cr2 = get_cr2();

    thread = current_thread();

    if ( thread == NULL ) {
        invalid_page_fault( NULL, regs, cr2, "unknown" );
    }

    LOCK( region_lock );

    region = do_memory_context_get_region_for( thread->process->memory_context, cr2 );

    if ( region == NULL ) {
        message = "no region for address";
        goto invalid;
    }

    if ( region->alloc_method == ALLOC_LAZY ) {
        uint32_t pages_loaded;

        if ( region->file == NULL ) {
            error = handle_lazy_page_allocation( region, cr2 );

            pages_loaded = 1;
        } else {
            error = handle_lazy_page_loading( region, cr2, &pages_loaded );
        }

        if ( error < 0 ) {
            message = "lazy page allocation failed";
            goto invalid;
        }

        /* Update pmem of the current process */

        spinlock_disable( &scheduler_lock );

        thread->process->pmem_size += pages_loaded * PAGE_SIZE;

        spinunlock_enable( &scheduler_lock );
    } else {
        uint32_t* pgd_entry;
        uint32_t* pt_entry;

        i386_memory_context_t* arch_context;

        memory_region_dump( region, -1 );

        arch_context = ( i386_memory_context_t* )thread->process->memory_context->arch_data;
        pgd_entry = page_directory_entry( arch_context, cr2 );

        kprintf( "Page directory entry: %x\n", *pgd_entry );

        if ( *pgd_entry != 0 ) {
            pt_entry = page_table_entry( *pgd_entry, cr2 );

            kprintf( "Page table entry: %x\n", *pt_entry );
        }

        message = "unknown";

        goto invalid;
    }

    UNLOCK( region_lock );

    return 0;

invalid:
    UNLOCK( region_lock );

    invalid_page_fault( thread, regs, cr2, message );

    return 0;
}
