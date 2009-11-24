/* Memory page allocator
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
#include <kernel.h>
#include <errno.h>
#include <bootmodule.h>
#include <macros.h>
#include <config.h>
#include <mm/pages.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/mm/config.h>

uint64_t memory_size;
memory_page_t* memory_pages;
spinlock_t pages_lock = INIT_SPINLOCK( "page allocator" );
memory_type_desc_t memory_descriptors[ MAX_MEMORY_TYPES ];

static inline memory_type_desc_t* get_memory_descriptor( ptr_t address ) {
    int i;
    memory_type_desc_t* memory_desc;

    for ( i = 0; i < MAX_MEMORY_TYPES; i++ ) {
        memory_desc = &memory_descriptors[ i ];

        if ( memory_desc->free ) {
            continue;
        }

        if ( ( address >= memory_desc->start ) &&
             ( ( address - memory_desc->start ) < memory_desc->size ) ) {
            return memory_desc;
        }
    }

    return NULL;
}

void* do_alloc_pages( memory_type_desc_t* memory_desc, uint32_t count ) {
    ptr_t i;
    void* p;
    bool found;
    memory_page_t* page;
    ptr_t page_count;
    ptr_t region_start;
    uint32_t free_page_count;

    page_count = memory_desc->size / PAGE_SIZE;

    /* First make sure this memory region is big enough for
       the requested number of pages */

    if ( __unlikely( page_count < count ) ) {
        return NULL;
    }

    p = NULL;
    found = false;
    page = &memory_pages[ memory_desc->start / PAGE_SIZE ];
    region_start = 0;
    free_page_count = 0;

    for ( i = 0; i < page_count; i++, page++ ) {
        if ( page->ref_count == 0 ) {
            if ( !found ) {
                found = true;
                region_start = i;
                free_page_count = 1;
            } else {
                free_page_count++;
            }

            if ( free_page_count == count ) {
                uint32_t j;
                memory_page_t* tmp;

                tmp = &memory_pages[ ( memory_desc->start / PAGE_SIZE ) + region_start ];

                for ( j = 0; j < count; j++, tmp++ ) {
                    tmp->ref_count = 1;
                }

                p = ( void* )( memory_desc->start + ( region_start * PAGE_SIZE ) );

                break;
            }
        } else {
            found = false;
        }
    }

    return p;
}

void* alloc_pages( uint32_t count, int mem_type ) {
    void* p;
    memory_type_desc_t* memory_desc;

    ASSERT( ( mem_type >= 0 ) && ( mem_type < MAX_MEMORY_TYPES ) );

    memory_desc = &memory_descriptors[ mem_type ];

    ASSERT( !memory_desc->free );

    spinlock_disable( &pages_lock );

    p = do_alloc_pages( memory_desc, count );

    spinunlock_enable( &pages_lock );

    return p;
}

static void* do_alloc_pages_aligned( memory_type_desc_t* memory_desc, uint32_t count, uint32_t alignment ) {
    ptr_t i;
    void* p;
    uint32_t j;
    ptr_t start;
    memory_page_t* page;
    uint32_t step;
    ptr_t start_page;
    ptr_t end_page;

    start = memory_desc->start;

    if ( ( start % alignment ) != 0 ) {
        start = ALIGN( start, alignment );
    }

    step = ALIGN( count * PAGE_SIZE, alignment ) / PAGE_SIZE;
    start_page = ( start - memory_desc->start ) / PAGE_SIZE;
    end_page = memory_desc->size / PAGE_SIZE;

    p = NULL;
    page = &memory_pages[ start / PAGE_SIZE ];

    for ( i = start_page; i < end_page; i += step ) {
        bool free = true;

        for ( j = 0; j < count; j++ ) {
            if ( memory_pages[ i + j ].ref_count != 0 ) {
                free = false;
                break;
            }
        }

        if ( free ) {
            for ( j = 0; j < count; j++ ) {
                memory_pages[ i + j ].ref_count = 1;
            }

            p = ( void* )( memory_desc->start + ( i * PAGE_SIZE ) );

            break;
        }
    }

    return p;
}

void* alloc_pages_aligned( uint32_t count, int mem_type, uint32_t alignment ) {
    void* p;
    memory_type_desc_t* memory_desc;

    ASSERT( ( mem_type >= 0 ) && ( mem_type < MAX_MEMORY_TYPES ) );
    ASSERT( ( alignment % PAGE_SIZE ) == 0 );

    memory_desc = &memory_descriptors[ mem_type ];

    ASSERT( !memory_desc->free );

    spinlock_disable( &pages_lock );

    p = do_alloc_pages_aligned( memory_desc, count, alignment );

    spinunlock_enable( &pages_lock );

    return p;
}

void free_pages( void* address, uint32_t count ) {
    uint32_t i;
    memory_page_t* tmp;
    ptr_t region_start;
    memory_type_desc_t* memory_desc;

    ASSERT( address != NULL );

    region_start = ( ptr_t )address;

    if ( __unlikely( region_start >= memory_size ) ) {
        panic( "free_pages(): Called with an address outside of physical memory!\n" );
        return;
    }

    region_start /= PAGE_SIZE;
    tmp = &memory_pages[ region_start ];
    memory_desc = get_memory_descriptor( ( ptr_t )address );

    ASSERT( memory_desc != NULL );

    spinlock_disable( &pages_lock );

    ASSERT( tmp->ref_count > 0 );

    for ( i = 0; i < count; i++, tmp++ ) {
        tmp->ref_count = 0;
    }

    spinunlock_enable( &pages_lock );
}

uint32_t get_free_page_count( void ) {
    uint32_t i;
    uint32_t count = 0;
    memory_page_t* page;

    spinlock_disable( &pages_lock );

    for ( i = 0, page = memory_pages; i < ( memory_size / PAGE_SIZE ); i++, page++ ) {
        if ( page->ref_count == 0 ) {
            count++;
        }
    }

    spinunlock_enable( &pages_lock );

    return count;
}

uint32_t get_total_page_count( void ) {
    return ( memory_size / PAGE_SIZE );
}

int sys_get_memory_info( memory_info_t* info ) {
    info->free_page_count = get_free_page_count();
    info->total_page_count = get_total_page_count();

    kmalloc_get_statistics(
        &info->kmalloc_used_pages,
        &info->kmalloc_alloc_size
    );

    return 0;
}

int reserve_memory_pages( ptr_t start, ptr_t size ) {
    ptr_t i;
    ptr_t count;
    memory_page_t* page;
    memory_type_desc_t* memory_desc;

    ASSERT( ( start % PAGE_SIZE ) == 0 );
    ASSERT( ( size % PAGE_SIZE ) == 0 );
    ASSERT( ( start + size ) <= memory_size );

    count = size / PAGE_SIZE;
    page = &memory_pages[ start / PAGE_SIZE ];
    memory_desc = get_memory_descriptor( start );

    ASSERT( memory_desc != NULL );
    ASSERT( ( start - memory_desc->start + size ) <= memory_desc->size );

    for ( i = 0; i < count; i++, page++ ) {
        if ( page->ref_count == 0 ) {
            page->ref_count++;
        }
    }

    return 0;
}

__init int register_memory_type( int mem_type, ptr_t start, ptr_t size ) {
    memory_type_desc_t* memory_desc;

    if ( ( mem_type < 0 ) || ( mem_type >= MAX_MEMORY_TYPES ) ) {
        return -EINVAL;
    }

    memory_desc = &memory_descriptors[ mem_type ];

    if ( __unlikely( !memory_desc->free ) ) {
        return -EBUSY;
    }

    memory_desc->free = false;
    memory_desc->start = start;
    memory_desc->size = size;

    return 0;
}

__init int init_page_allocator( ptr_t page_map_address, uint64_t _memory_size ) {
    int i;
    ptr_t page_count;
    memory_type_desc_t* memory_desc;

    /* Make all memory types free */

    for ( i = 0; i < MAX_MEMORY_TYPES; i++ ) {
        memory_desc = &memory_descriptors[ i ];

        memory_desc->free = true;
        memory_desc->start = 0;
        memory_desc->size = 0;
    }

    /* Setup the page structures for the whole memory */

    memory_size = _memory_size;
    memory_pages = ( memory_page_t* )page_map_address;

    page_count = memory_size / PAGE_SIZE;

    memset( ( void* )memory_pages, 0, page_count * sizeof( memory_page_t ) );

    return 0;
}

__init int init_page_allocator_late( void ) {
    /* Reserve the memory pages used for the memory_page_t structures */

    reserve_memory_pages( ( ptr_t )memory_pages, PAGE_ALIGN( ( memory_size / PAGE_SIZE ) * sizeof( memory_page_t ) ) );

    return 0;
}
