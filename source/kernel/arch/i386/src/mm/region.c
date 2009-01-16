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
#include <mm/pages.h>

#include <arch/cpu.h>
#include <arch/mm/region.h>
#include <arch/mm/context.h>
#include <arch/mm/paging.h>

int arch_create_region_pages( memory_context_t* context, region_t* region ) {
    int error;
    bool do_tlb_flush;
    i386_memory_context_t* arch_context;

    do_tlb_flush = true;
    arch_context = ( i386_memory_context_t* )context->arch_data;

    /* Create the physical pages for the memory region */

    switch ( ( int )region->alloc_method ) {
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

            break;

        case ALLOC_CONTIGUOUS : {
            void* p;

            p = alloc_pages( region->size / PAGE_SIZE );

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

            break;
        }

        case ALLOC_LAZY :
            panic( "Lazy page allocation not yet supported!\n" );
            break;

        default :
            do_tlb_flush = false;
            break;
    }

    /* Invalidate the TLB if we changed anything that requires it */

    if ( do_tlb_flush ) {
        flush_tlb();
    }

    return 0;
}

int arch_delete_region_pages( memory_context_t* context, region_t* region ) {
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    if ( ( region->flags & REGION_REMAPPED ) == 0 ) {
        switch ( ( int )region->alloc_method ) {
            case ALLOC_LAZY :
                panic( "Freeing lazy allocated region not yet implemented!\n" );
                break;

            case ALLOC_PAGES :
                free_region_pages( arch_context, region->start, region->size );
                break;

            case ALLOC_CONTIGUOUS :
                free_region_pages_contiguous( arch_context, region->start, region->size );
                break;
        }
    } else {
        free_region_pages_remapped( arch_context, region->start, region->size );
    }

    free_region_page_tables( arch_context, region->start, region->size );

    flush_tlb();

    return 0;
}

int arch_resize_region( struct memory_context* context, region_t* region, uint32_t new_size ) {
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    if ( new_size < region->size ) {
        free_region_pages( arch_context, region->start + new_size, region->size - new_size );
        free_region_page_tables( arch_context, region->start + new_size, region->size - new_size );
    } else {
        switch ( ( int )region->alloc_method ) {
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

                break;
            }

            case ALLOC_CONTIGUOUS :
                panic( "Resizing contiguous allocated memory region not yet implemented!\n" );
                break;
        }
    }

    flush_tlb();

    return 0;
}
