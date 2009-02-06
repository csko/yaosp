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
#include <scheduler.h>
#include <process.h>
#include <mm/pages.h>

#include <arch/smp.h>
#include <arch/mm/region.h>
#include <arch/mm/context.h>
#include <arch/mm/paging.h>

int arch_create_region_pages( memory_context_t* context, region_t* region ) {
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
        spinlock_disable( &scheduler_lock );

        context->process->pmem_size += allocated_pmem;

        spinunlock_enable( &scheduler_lock );
    }

    /* Invalidate the TLB if we changed anything that requires it */

    if ( do_tlb_flush ) {
        flush_tlb_global();
    }

    return 0;
}

int arch_delete_region_pages( memory_context_t* context, region_t* region ) {
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
        spinlock_disable( &scheduler_lock );

        context->process->pmem_size -= freed_pmem;

        spinunlock_enable( &scheduler_lock );
    }

    flush_tlb_global();

    return 0;
}

int arch_remap_region_pages( struct memory_context* context, region_t* region, ptr_t address ) {
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

int arch_resize_region( struct memory_context* context, region_t* region, uint32_t new_size ) {
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    if ( new_size < region->size ) {
        free_region_pages( arch_context, region->start + new_size, region->size - new_size );
        free_region_page_tables( arch_context, region->start + new_size, region->size - new_size );

        spinlock_disable( &scheduler_lock );

        context->process->pmem_size -= ( region->size - new_size );

        spinunlock_enable( &scheduler_lock );
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
            spinlock_disable( &scheduler_lock );

            context->process->pmem_size += allocated_pmem;

            spinunlock_enable( &scheduler_lock );
        }
    }

    flush_tlb_global();

    return 0;
}

int arch_clone_memory_region(
    memory_context_t* old_context,
    region_t* old_region,
    memory_context_t* new_context,
    region_t* new_region
) {
    int error;

    if ( old_region->flags & REGION_KERNEL ) {
        error = 0;
    } else {
        error = clone_user_region( old_context, old_region, new_context, new_region );
    }

    return error;
}
