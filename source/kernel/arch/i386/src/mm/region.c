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
#include <sched/scheduler.h>
#include <mm/pages.h>

#include <arch/smp.h>
#include <arch/mm/region.h>
#include <arch/mm/context.h>
#include <arch/mm/paging.h>

int arch_create_region_pages( memory_context_t* context, memory_region_t* region ) {
    int error;
    bool do_tlb_flush;
    uint64_t allocated_pmem;
    i386_memory_context_t* arch_context;

    allocated_pmem = 0;
    do_tlb_flush = true;
    arch_context = ( i386_memory_context_t* )context->arch_data;

    /* Create the physical pages for the memory region */

    switch ( region->alloc_method ) {
        case ALLOC_PAGES :
            error = create_region_pages(
                arch_context,
                region->start,
                region->size,
                ( region->flags & REGION_KERNEL ) != 0,
                ( region->flags & REGION_WRITE ) != 0
            );

            if ( error < 0 ) {
                return error;
            }

            allocated_pmem = region->size;

            break;

        case ALLOC_CONTIGUOUS : {
            void* p;

            p = alloc_pages( region->size / PAGE_SIZE, MEM_COMMON );

            if ( p == NULL ) {
                return -ENOMEM;
            }

            error = map_region_pages(
                arch_context,
                region->start,
                ( ptr_t )p,
                region->size,
                ( region->flags & REGION_KERNEL ) != 0,
                ( region->flags & REGION_WRITE ) != 0
            );

            if ( error < 0 ) {
                return error;
            }

            allocated_pmem = region->size;

            break;
        }

        case ALLOC_LAZY :
            error = clear_region_pages(
                arch_context,
                region->start,
                region->size,
                false
            );

            if ( error < 0 ) {
                return error;
            }

            break;

        case ALLOC_NONE :
            do_tlb_flush = false;
            break;
    }

    /* Update pmem statistics */

    if ( allocated_pmem != 0 ) {
        scheduler_lock();

        context->process->pmem_size += allocated_pmem;

        scheduler_unlock();
    }

    /* Invalidate the TLB if we changed anything that requires it */

    if ( do_tlb_flush ) {
        flush_tlb_global();
    }

    return 0;
}

int arch_delete_region_pages( memory_context_t* context, memory_region_t* region ) {
    uint64_t freed_pmem;
    i386_memory_context_t* arch_context;

    freed_pmem = 0;
    arch_context = ( i386_memory_context_t* )context->arch_data;

    if ( ( region->flags & REGION_REMAPPED ) == 0 ) {
        switch ( region->alloc_method ) {
            case ALLOC_PAGES :
                free_region_pages( arch_context, region->start, region->size );
                freed_pmem = region->size;
                break;

            case ALLOC_CONTIGUOUS :
                free_region_pages_contiguous( arch_context, region->start, region->size );
                freed_pmem = region->size;
                break;

            case ALLOC_LAZY :
                free_region_pages_lazy( arch_context, region->start, region->size );
                break;

            case ALLOC_NONE :
                break;
        }
    } else {
        free_region_pages_remapped( arch_context, region->start, region->size );
    }

    free_region_page_tables( arch_context, region->start, region->size );

    /* Update pmem statistics */

    if ( freed_pmem != 0 ) {
        scheduler_lock();

        context->process->pmem_size -= freed_pmem;

        scheduler_unlock();
    }

    flush_tlb_global();

    return 0;
}

int arch_remap_region_pages( struct memory_context* context, memory_region_t* region, ptr_t address ) {
    int error;
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    error = map_region_pages(
        arch_context,
        region->start,
        address,
        region->size,
        ( region->flags & REGION_KERNEL ) != 0,
        ( region->flags & REGION_WRITE ) != 0
    );

    if ( error < 0 ) {
        return error;
    }

    flush_tlb_global();

    return 0;
}

int arch_resize_region( struct memory_context* context, memory_region_t* region, uint32_t new_size ) {
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    if ( new_size < region->size ) {
        free_region_pages( arch_context, region->start + new_size, region->size - new_size );
        free_region_page_tables( arch_context, region->start + new_size, region->size - new_size );

        scheduler_lock();

        context->process->pmem_size -= ( region->size - new_size );

        scheduler_unlock();
    } else {
        uint64_t allocated_pmem = 0;

        switch ( region->alloc_method ) {
            case ALLOC_LAZY :
                panic( "Resizing lazy allocated memory region not yet implemented!\n" );
                break;

            case ALLOC_PAGES : {
                int error;

                error = create_region_pages(
                    arch_context,
                    region->start + region->size,
                    new_size - region->size,
                    ( region->flags & REGION_KERNEL ) != 0,
                    ( region->flags & REGION_WRITE ) != 0
                );

                if ( error < 0 ) {
                    return error;
                }

                allocated_pmem = new_size - region->size;

                break;
            }

            case ALLOC_CONTIGUOUS :
                panic( "Resizing contiguous allocated memory region not yet implemented!\n" );
                break;

            case ALLOC_NONE :
                break;
        }

        if ( allocated_pmem != 0 ) {
            scheduler_lock();

            context->process->pmem_size += allocated_pmem;

            scheduler_unlock();
        }
    }

    flush_tlb_global();

    return 0;
}

int arch_clone_region( memory_region_t* old_region, memory_region_t* new_region ) {
    i386_memory_context_t* old_context;
    i386_memory_context_t* new_context;

    old_context = ( i386_memory_context_t* )old_region->context->arch_data;
    new_context = ( i386_memory_context_t* )new_region->context->arch_data;

    int error;
    uint32_t i;
    uint32_t count;
    uint32_t old_pd_index;
    uint32_t old_pt_index;
    uint32_t new_pd_index;
    uint32_t new_pt_index;
    uint32_t* old_pt;
    uint32_t* new_pt;

    /* Create the page tables if required */

    error = map_region_page_tables( new_context, new_region->start, new_region->size, false );

    if ( error < 0 ) {
        return error;
    }

    /* Do the page mapping */

    count = new_region->size / PAGE_SIZE;
    old_pd_index = old_region->start >> PGDIR_SHIFT;
    old_pt_index = ( old_region->start >> PAGE_SHIFT ) & 1023;
    old_pt = ( uint32_t* )( old_context->page_directory[ old_pd_index ] & PAGE_MASK );
    new_pd_index = new_region->start >> PGDIR_SHIFT;
    new_pt_index = ( new_region->start >> PAGE_SHIFT ) & 1023;
    new_pt = ( uint32_t* )( new_context->page_directory[ new_pd_index ] & PAGE_MASK );

    for ( i = 0; i < count; i++ ) {
        new_pt[ new_pt_index ] = old_pt[ old_pt_index ];

        if ( old_pt_index == 1023 ) {
            old_pt_index = 0;
            old_pd_index++;

            old_pt = ( uint32_t* )( old_context->page_directory[ old_pd_index ] & PAGE_MASK );
        } else {
            old_pt_index++;
        }

        if ( new_pt_index == 1023 ) {
            new_pt_index = 0;
            new_pd_index++;

            new_pt = ( uint32_t* )( new_context->page_directory[ new_pd_index ] & PAGE_MASK );
        } else {
            new_pt_index++;
        }
    }

    return 0;
}

int arch_clone_memory_region(
    memory_context_t* old_context,
    memory_region_t* old_region,
    memory_context_t* new_context,
    memory_region_t* new_region
) {
    ASSERT( ( old_region->flags & REGION_KERNEL ) == 0 );

    return clone_user_region( old_context, old_region, new_context, new_region );
}
