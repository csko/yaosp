/* Memory page allocator
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
 *
 * Memory structure:
 *
 * -------------- <- 0 Mb
 * |    Below   |
 * |     1Mb    |
 * |------------| <- 1 Mb
 * |   Kernel   |
 * |------------| <- 1 Mb + kernel_size
 * |    Page    |
 * | allocator  |
 * | structures |
 * |------------| <- 1 Mb + kernel_size +
 * |            |    sizeof(page_t) * memory_pages
 * |   Kernel   |
 * |   memory   |
 * |            |
 * |------------| <- userspace start
 * |     U      |
 * |     S      |
 * |     E      |
 * |     R      |
 * |   SPACE    |
 * |------------| <- end of memory region
 */

#include <types.h>
#include <kernel.h>
#include <errno.h>
#include <mm/pages.h>
#include <lib/string.h>

#include <arch/atomic.h>
#include <arch/spinlock.h>
#include <arch/mm/config.h>

ptr_t memory_size;
page_t* memory_pages;

atomic_t free_page_count;
uint32_t memory_page_count;

ptr_t first_free_page_index;

static spinlock_t pages_lock = INIT_SPINLOCK;

extern int __kernel_end;

void* alloc_pages( uint32_t count ) {
    void* p = NULL;
    ptr_t i;
    ptr_t region_start = 0;
    uint32_t free_pages = 0;

    spinlock_disable( &pages_lock );

    for ( i = first_free_page_index; i < memory_page_count; i++ ) {
        if ( atomic_get( &memory_pages[ i ].ref ) == 0 ) {
            if ( region_start == 0 ) {
                region_start = i;
                free_pages = 1;
            } else {
                free_pages++;
            }

            if ( free_pages == count ) {
                uint32_t j;
                page_t* tmp = &memory_pages[ region_start ];

                for ( j = 0; j < count; j++, tmp++ ) {
                    atomic_set( &tmp->ref, 1 );
                    atomic_dec( &free_page_count );
                }

                p = ( void* )( region_start * PAGE_SIZE );

                break;
            }
        } else {
            region_start = 0;
        }
    }

    spinunlock_enable( &pages_lock );

    return p;
}

void free_pages( void* address, uint32_t count ) {
    uint32_t i;
    page_t* tmp;
    ptr_t region_start;

    region_start = ( ptr_t )address;

    if ( region_start >= memory_size ) {
        panic( "free_pages(): Called with an address outside of physical memory!\n" );
        return;
    }

    region_start /= PAGE_SIZE;
    tmp = &memory_pages[ region_start ];

    spinlock_disable( &pages_lock );

    if ( atomic_get( &tmp->ref ) == 0 ) {
        panic( "free_pages(): Tried to free a non-allocated region!\n" );
        goto out;
    }

    for ( i = 0; i < count; i++, tmp++ ) {
        atomic_set( &tmp->ref, 0 );
        atomic_inc( &free_page_count );
    }

out:
    spinunlock_enable( &pages_lock );
}

int get_free_page_count( void ) {
    return atomic_get( &free_page_count );
}

int init_page_allocator( multiboot_header_t* header ) {
    ptr_t i;
    ptr_t first_free_page;

    /* Check if we have memory information in the multiboot header */

    if ( ( header->flags & MB_FLAG_MEMORY_INFO ) == 0 ) {
        return -EINVAL;
    }

    /* Calculate the physical memory size and the number
       of usable memory pages */

    memory_size = header->memory_upper * 1024 + 1024 * 1024;
    memory_page_count = memory_size / PAGE_SIZE;
    atomic_set( &free_page_count, memory_page_count );

    /* The first usable page starts at the end of the kernel.
       We place the page_t structures right after the kernel. */

    first_free_page = PAGE_ALIGN( ( ptr_t )&__kernel_end );
    memory_pages = ( page_t* )first_free_page;

    /* Reserve the pages used by the page allocator structures */

    first_free_page += PAGE_ALIGN( memory_page_count * sizeof( page_t ) );
    first_free_page_index = first_free_page / PAGE_SIZE;

    /* Clear the memory */

    memset( ( void* )memory_pages, 0, memory_page_count * sizeof( page_t ) );

    /* Make the kernel and page allocator pages used not to
       allocate from this region. */

    for ( i = 0; i < first_free_page; i += PAGE_SIZE ) {
      atomic_inc( &memory_pages[ i ].ref );
      atomic_dec( &free_page_count );
    }

    return 0;
}
