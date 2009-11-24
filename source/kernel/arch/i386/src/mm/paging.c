/* i386 paging code
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

#include <errno.h>
#include <types.h>
#include <kernel.h>
#include <macros.h>
#include <console.h>
#include <sched/scheduler.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <mm/region.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/mm/paging.h>

static i386_memory_context_t i386_kernel_memory_context;

int get_paging_flags_for_region( memory_region_t* region ) {
    register uint32_t flags;

    flags = PAGE_PRESENT;

    if ( region->flags & REGION_WRITE ) {
        flags |= PAGE_WRITE;
    }

    if ( ( region->flags & REGION_KERNEL ) == 0 ) {
        flags |= PAGE_USER;
    }

    return flags;
}

int paging_alloc_table_entries( uint32_t* table, uint32_t from, uint32_t to, uint32_t flags ) {
    uint32_t i;

    ASSERT( ( to >= 0 ) && ( to <= 1023 ) );
    ASSERT( from <= to );

    for ( i = from; i <= to; i++ ) {
        void* p;

        /* If it's already allocated, just skip the entry */

        if ( table[ i ] != 0 ) {
            continue;
        }

        p = alloc_pages( 1, MEM_COMMON );

        if ( p == NULL ) {
            return -ENOMEM;
        }

        memsetl( p, 0, PAGE_SIZE / 4 );

        table[ i ] = ( uint32_t )p | flags;
    }

    return 0;
}

int paging_fill_table_entries( uint32_t* table, uint32_t address, uint32_t from, uint32_t to, uint32_t flags ) {
    uint32_t i;

    ASSERT( ( to >= 0 ) && ( to <= 1023 ) );
    ASSERT( from <= to );

    address |= flags;

    for ( i = from; i <= to; i++, address += PAGE_SIZE ) {
        if ( __unlikely( table[ i ] != 0 ) ) {
            kprintf(
                WARNING,
                "fill_table_entries(): Table (%p) entry %d is not empty: %x!\n",
                table, i, table[ i ]
            );

            return -1;
        }

        table[ i ] = address;
    }

    return 0;
}

int paging_clear_table_entries( uint32_t* table, uint32_t from, uint32_t to ) {
    uint32_t i;

    ASSERT( ( to >= 0 ) && ( to <= 1023 ) );
    ASSERT( from <= to );

    for ( i = from; i <= to; i++ ) {
        table[ i ] = 0;
    }

    return 0;
}

int paging_free_table_entries( uint32_t* table, uint32_t from, uint32_t to ) {
    uint32_t i;

    ASSERT( ( to >= 0 ) && ( to <= 1023 ) );
    ASSERT( from <= to );

    for ( i = from; i <= to; i++ ) {
        uint32_t ptr;

        if ( table[ i ] == 0 ) {
            continue;
        }

        ptr = table[ i ] & PAGE_MASK;

        if ( ptr < memory_size ) {
            memory_page_t* page = &memory_pages[ ptr / PAGE_SIZE ];

            ASSERT( page->ref_count > 0 );
            page->ref_count--;
        }

        table[ i ] = 0;
    }

    return 0;
}

int paging_clone_table_entries( uint32_t* old_table, uint32_t* new_table,
                                uint32_t from, uint32_t to, int remove_write ) {
    uint32_t i;

    ASSERT( ( to >= 0 ) && ( to <= 1023 ) );
    ASSERT( from <= to );

    for ( i = from; i <= to; i++ ) {
        uint32_t ptr;
        memory_page_t* page;

        /* Remove write permission if needed and copy the address of the page */

        if ( remove_write ) {
            old_table[ i ] &= ~PAGE_WRITE;
        }

        new_table[ i ] = old_table[ i ];

        /* Increase the ref count of the page */

        ptr = old_table[ i ] & PAGE_MASK;
        ASSERT( ptr < memory_size );

        page = &memory_pages[ ptr / PAGE_SIZE ];
        ASSERT( page->ref_count > 0 );
        page->ref_count++;
    }

    return 0;
}

__init int init_paging( void ) {
    int error;
    uint32_t size;
    register_t dummy;
    memory_region_t* region;
    memory_context_t* context;
    i386_memory_context_t* arch_context;

    /* Initialize kernel memory context */

    context = &kernel_memory_context;
    arch_context = &i386_kernel_memory_context;

    /* .. we can't use memory_context_init() here because
       that function uses locking related calls but the
       locking part of the kernel is not yet initialized. */

    memset( context, 0, sizeof( memory_context_t ) );
    memset( arch_context, 0, sizeof( i386_memory_context_t ) );

    error = array_init( &context->regions );

    if ( error < 0 ) {
        return error;
    }

    array_set_realloc_size( &context->regions, 32 );
    context->mutex = -1;
    context->next = context;
    context->arch_data = arch_context;

    /* Allocate page directory */

    arch_context->page_directory = ( uint32_t* )alloc_pages( 1, MEM_COMMON );

    if ( arch_context->page_directory == NULL ) {
        return -ENOMEM;
    }

    memsetl( arch_context->page_directory, 0, PAGE_SIZE / 4 );

    /* Map the screen */

    region = do_create_memory_region_at(
        context,
        "screen",
        0xB8000,
        PAGE_SIZE,
        REGION_READ | REGION_WRITE | REGION_KERNEL
    );

    do_memory_region_remap_pages( region, 0xB8000 );
    memory_context_insert_region( context, region );

    /* Map the read-only part of the kernel */

    size = ( uint32_t )&__ro_end - 0x100000;

    region = do_create_memory_region_at(
        context,
        "kernel-ro",
        0x100000,
        size,
        REGION_READ | REGION_KERNEL
    );

    do_memory_region_remap_pages( region, 0x100000 );
    memory_context_insert_region( context, region );

    /* Map the read-write part of the kernel */

    size = ( uint32_t )&__kernel_end - ( uint32_t )&__data_start;

    region = do_create_memory_region_at(
        context,
        "kernel-rw",
        ( ptr_t )&__data_start,
        size,
        REGION_READ | REGION_WRITE | REGION_KERNEL
    );

    do_memory_region_remap_pages( region, ( ptr_t )&__data_start );
    memory_context_insert_region( context, region );

    /* Map the kernel heap */

    size = MIN( get_total_page_count() * PAGE_SIZE,
                512 * 1024 * 1024 ) - ( 1 * 1024 * 1024 + ( ( uint32_t )__kernel_end - 0x100000 ) );

    region = do_create_memory_region_at(
        context,
        "kernel-heap",
        ( ptr_t )&__kernel_end,
        size,
        REGION_READ | REGION_WRITE | REGION_KERNEL
    );

    do_memory_region_remap_pages( region, ( ptr_t )&__kernel_end );
    memory_context_insert_region( context, region );

    /* Load CR3 */

    set_cr3( ( uint32_t )arch_context->page_directory );

    /* Enable paging */

    __asm__ __volatile__(
        "movl %%cr0, %0\n"
        "orl $0x80000000, %0\n"
        "movl %0, %%cr0\n"
        : "=r" ( dummy )
    );

    return 0;
}
