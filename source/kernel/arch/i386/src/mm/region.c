/* Memory region handling
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

#include <errno.h>
#include <kernel.h>
#include <process.h>
#include <macros.h>
#include <console.h>
#include <sched/scheduler.h>
#include <mm/pages.h>
#include <lib/string.h>

#include <arch/smp.h>
#include <arch/interrupt.h>
#include <arch/mm/region.h>
#include <arch/mm/context.h>
#include <arch/mm/paging.h>

int arch_memory_region_remap_pages( memory_region_t* region, ptr_t physical_address ) {
    int error;
    uint32_t curr_pt;
    uint32_t last_pt;
    uint32_t paging_flags;
    uint32_t* page_directory;
    i386_memory_context_t* arch_context;

    paging_flags = get_paging_flags_for_region( region );
    arch_context = ( i386_memory_context_t* )region->context->arch_data;
    page_directory = arch_context->page_directory;

    /* Allocate possibly missing page tables */

    curr_pt = PGD_INDEX( region->address );
    last_pt = PGD_INDEX( region->address + region->size - 1 );

    error = paging_alloc_table_entries(
        page_directory,
        curr_pt,
        last_pt,
        paging_flags | PAGE_WRITE
    );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    /* Map the pages of the region */

    uint32_t first_page = PT_INDEX( region->address );
    uint32_t last_page = ( curr_pt == last_pt ? PT_INDEX( region->address + region->size - 1 ) : 1023 );

    error = paging_fill_table_entries(
        ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK ),
        physical_address, first_page,
        last_page, paging_flags
    );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    curr_pt++;
    physical_address += ( last_page - first_page + 1 ) * PAGE_SIZE;

    for ( ; curr_pt < last_pt; curr_pt++ ) {
        error = paging_fill_table_entries(
            ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK ),
            physical_address, 0,
            1023, paging_flags
        );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }

        physical_address += 1024 * PAGE_SIZE;
    }

    if ( curr_pt == last_pt ) {
        error = paging_fill_table_entries(
            ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK ),
            physical_address, 0,
            PT_INDEX( region->address + region->size - 1 ), paging_flags
        );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }
    }

    return 0;
}

int arch_memory_region_alloc_pages( memory_region_t* region, ptr_t virtual, uint64_t size ) {
    int error;
    uint32_t curr_pt;
    uint32_t last_pt;
    uint32_t paging_flags;
    uint32_t* page_directory;
    i386_memory_context_t* arch_context;

    paging_flags = get_paging_flags_for_region( region );
    arch_context = ( i386_memory_context_t* )region->context->arch_data;
    page_directory = arch_context->page_directory;

    /* Allocate possibly missing page tables */

    curr_pt = PGD_INDEX( virtual );
    last_pt = PGD_INDEX( virtual + size - 1 );

    error = paging_alloc_table_entries(
        page_directory, curr_pt,
        last_pt, paging_flags | PAGE_WRITE
    );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    /* Allocate the pages of the region */

    uint32_t first_page = PT_INDEX( virtual );
    uint32_t last_page = ( curr_pt == last_pt ? PT_INDEX( virtual + size - 1 ) : 1023 );

    error = paging_alloc_table_entries(
        ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK ),
        first_page, last_page, paging_flags
    );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    curr_pt++;

    for ( ; curr_pt < last_pt; curr_pt++ ) {
        error = paging_alloc_table_entries(
            ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK ),
            0, 1023, paging_flags
        );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }
    }

    if ( curr_pt == last_pt ) {
        error = paging_alloc_table_entries(
            ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK ),
            0, PT_INDEX( virtual + size - 1 ), paging_flags
        );

        if ( __unlikely( error < 0 ) ) {
            return error;
        }
    }

    return 0;
}

typedef int table_unmap_func_t( uint32_t* table, uint32_t from, uint32_t to );

int arch_memory_region_unmap_pages( memory_region_t* region, ptr_t virtual, uint64_t size ) {
    uint32_t* pd;
    uint32_t curr_pt;
    uint32_t last_pt;
    uint32_t* page_directory;
    table_unmap_func_t* unmap_function;
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )region->context->arch_data;
    page_directory = arch_context->page_directory;

    switch ( region->flags & REGION_MAPPING_FLAGS ) {
        case REGION_REMAPPED :
            unmap_function = paging_clear_table_entries;
            break;

        case REGION_ALLOCATED :
        case REGION_CLONED :
            unmap_function = paging_free_table_entries;
            break;

        default :
            kprintf(
                WARNING, "arch_memory_region_unmap_pages(): invalid mapping mode: %x\n",
                region->flags & REGION_MAPPING_FLAGS
            );
            return -1;
    }

    curr_pt = PGD_INDEX( virtual );
    last_pt = PGD_INDEX( virtual + size - 1 );

    uint32_t first_page = PT_INDEX( virtual );
    uint32_t last_page = ( curr_pt == last_pt ? PT_INDEX( virtual + size - 1 ) : 1023 );

    /* Unmap the pages of the region */

    spinlock_disable( &pages_lock );

    pd = ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK );

    if ( pd != NULL ) {
        unmap_function( pd, first_page, last_page );
    }

    curr_pt++;

    for ( ; curr_pt < last_pt; curr_pt++ ) {
        pd = ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK );
        unmap_function( pd, 0, 1023 );
    }

    if ( curr_pt == last_pt ) {
        pd = ( uint32_t* )( page_directory[ curr_pt ] & PAGE_MASK );
        unmap_function( pd, 0, PT_INDEX( virtual + size - 1 ) );
    }

    spinunlock_enable( &pages_lock );

    flush_tlb();

    return 0;
}

static int do_clone_allocated_region_pages( memory_region_t* old_region, memory_region_t* new_region ) {
    int error;
    uint32_t curr_pt;
    uint32_t last_pt;
    uint32_t paging_flags;
    uint32_t* old_page_directory;
    uint32_t* new_page_directory;
    i386_memory_context_t* arch_context;

    paging_flags = get_paging_flags_for_region( new_region );

    arch_context = ( i386_memory_context_t* )old_region->context->arch_data;
    old_page_directory = arch_context->page_directory;
    arch_context = ( i386_memory_context_t* )new_region->context->arch_data;
    new_page_directory = arch_context->page_directory;

    /* Allocate possibly missing page tables */

    curr_pt = PGD_INDEX( new_region->address );
    last_pt = PGD_INDEX( new_region->address + new_region->size - 1 );

    error = paging_alloc_table_entries(
        new_page_directory,
        curr_pt,
        last_pt,
        paging_flags | PAGE_WRITE
    );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    int remove_write = ( ( new_region->flags & REGION_WRITE ) && ( ( new_region->flags & REGION_CLONED ) == 0 ) );
    uint32_t first_page = PT_INDEX( new_region->address );
    uint32_t last_page = ( curr_pt == last_pt ? PT_INDEX( new_region->address + new_region->size - 1 ) : 1023 );

    /* Clone the pages of the region */

    spinlock_disable( &pages_lock );

    paging_clone_table_entries(
        ( uint32_t* )( old_page_directory[ curr_pt ] & PAGE_MASK ),
        ( uint32_t* )( new_page_directory[ curr_pt ] & PAGE_MASK ),
        first_page, last_page, remove_write
    );

    curr_pt++;

    for ( ; curr_pt < last_pt; curr_pt++ ) {
        paging_clone_table_entries(
            ( uint32_t* )( old_page_directory[ curr_pt ] & PAGE_MASK ),
            ( uint32_t* )( new_page_directory[ curr_pt ] & PAGE_MASK ),
            0, 1023, remove_write
        );
    }

    if ( curr_pt == last_pt ) {
        paging_clone_table_entries(
            ( uint32_t* )( old_page_directory[ curr_pt ] & PAGE_MASK ),
            ( uint32_t* )( new_page_directory[ curr_pt ] & PAGE_MASK ),
            0, PT_INDEX( new_region->address + new_region->size - 1 ), remove_write
        );
    }

    spinunlock_enable( &pages_lock );

    /* We need to flush the TLB as we might remove write access
       from the pages of the currently running process. */

    if ( remove_write ) {
        flush_tlb();
    }

    return 0;
}

static int do_clone_region_pages( memory_region_t* old_region, memory_region_t* new_region ) {
    int error;
    uint32_t paging_flags;

    uint32_t* old_page_directory;
    uint32_t* new_page_directory;
    i386_memory_context_t* arch_context;

    paging_flags = get_paging_flags_for_region( new_region );

    arch_context = ( i386_memory_context_t* )old_region->context->arch_data;
    old_page_directory = arch_context->page_directory;
    arch_context = ( i386_memory_context_t* )new_region->context->arch_data;
    new_page_directory = arch_context->page_directory;

    /* Allocate possibly missing page tables */

    error = paging_alloc_table_entries(
        new_page_directory,
        PGD_INDEX( new_region->address ),
        PGD_INDEX( new_region->address + new_region->size - 1 ),
        paging_flags | PAGE_WRITE
    );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    /* Clone pages one-by-one */

    ptr_t old_address = old_region->address;
    ptr_t new_address = new_region->address;

    spinlock_disable( &pages_lock );

    for ( ;
          old_address < old_region->address + old_region->size;
          old_region += PAGE_SIZE, new_region += PAGE_SIZE ) {
        uint32_t* old_pt = ( uint32_t* )( old_page_directory[ PGD_INDEX( old_address ) ] & PAGE_MASK );
        uint32_t* new_pt = ( uint32_t* )( new_page_directory[ PGD_INDEX( new_address ) ] & PAGE_MASK );

        ptr_t ptr = old_pt[ PT_INDEX( old_address ) ];
        new_pt[ PT_INDEX( new_address ) ] = ptr;

        ptr &= PAGE_MASK;
        ASSERT( ptr < memory_size );
        memory_page_t* page = &memory_pages[ ptr / PAGE_SIZE ];
        ASSERT( page->ref_count > 0 );
        page->ref_count++;
    }

    spinunlock_enable( &pages_lock );

    return 0;
}

int arch_memory_region_clone_pages( memory_region_t* old_region, memory_region_t* new_region ) {
    ASSERT( old_region->size == new_region->size );

    switch ( new_region->flags & REGION_MAPPING_FLAGS ) {
        case REGION_REMAPPED :
            kprintf( WARNING, "%s(): REGION_REMAPPED not yet supported!\n", __FUNCTION__ );
            return -1;

        case REGION_ALLOCATED :
        case REGION_CLONED :
            if ( old_region->address == new_region->address ) {
                return do_clone_allocated_region_pages( old_region, new_region );
            };

            ASSERT( ( new_region->flags & REGION_MAPPING_FLAGS ) != REGION_ALLOCATED );
            return do_clone_region_pages( old_region, new_region );

        default :
            kprintf( WARNING, "%s(): Invalid mapping mode!\n", __FUNCTION__ );
            return -1;
    }
}
