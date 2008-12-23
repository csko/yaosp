/* i386 paging code
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
 */

#include <errno.h>
#include <types.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <mm/region.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/mm/paging.h>

static i386_memory_context_t i386_kernel_memory_context;

static int map_region_page_tables( i386_memory_context_t* arch_context, ptr_t start, uint32_t size ) {
    void* p;
    ptr_t addr;
    uint32_t* pgd_entry;

    for ( addr = start; addr < ( start + size ); addr += PGDIR_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );

        if ( *pgd_entry == 0 ) {
            p = alloc_pages( 1 );

            if ( p == NULL ) {
                return -ENOMEM;
            }

            memsetl( p, 0, PAGE_SIZE / 4 );

            *pgd_entry = ( ptr_t )p | PRESENT | WRITE;
        }
    }

    return 0;
}

static int map_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, ptr_t physical, uint32_t size ) {
    int error;
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;

    error = map_region_page_tables( arch_context, virtual, size );

    if ( error < 0 ) {
        return error;
    }

    for ( addr = virtual; addr < ( virtual + size ); addr += PAGE_SIZE, physical += PAGE_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );
        pt_entry = page_table_entry( *pgd_entry, addr );

        *pt_entry = physical | PRESENT | WRITE;
    }

    return 0;
}

static int create_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size ) {
    int error;
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;

    error = map_region_page_tables( arch_context, virtual, size );

    if ( error < 0 ) {
        return error;
    }

    for ( addr = virtual; addr < ( virtual + size ); addr += PAGE_SIZE ) {
        void* p;

        pgd_entry = page_directory_entry( arch_context, addr );
        pt_entry = page_table_entry( *pgd_entry, addr );

        p = alloc_pages( 1 );

        if ( p == NULL ) {
            return -ENOMEM;
        }

        *pt_entry = ( uint32_t )p | PRESENT | WRITE;
    }

    return 0;
}

int arch_create_region_pages( memory_context_t* context, region_t* region, alloc_type_t alloc_method ) {
    int error;
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    /* Create the physical pages for the memory region */

    switch ( ( int )alloc_method ) {
        case ALLOC_PAGES :
            error = create_region_pages( arch_context, region->start, region->size );

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

            error = map_region_pages( arch_context, region->start, ( ptr_t )p, region->size );

            if ( error < 0 ) {
                return error;
            }

            break;
        }
    }

    /* Invalidate TLB */

    flush_tlb();

    return 0;
}

int init_paging( void ) {
    int error;
    region_t* region;
    register_t dummy;
    memory_context_t* context;
    i386_memory_context_t* arch_context;

    /* Initialize kernel memory context */

    context = &kernel_memory_context;
    arch_context = &i386_kernel_memory_context;

    memset( context, 0, sizeof( memory_context_t ) );
    memset( arch_context, 0, sizeof( i386_memory_context_t ) );

    context->next = context;
    context->arch_data = arch_context;

    /* Allocate page directory */

    arch_context->page_directory = ( uint32_t* )alloc_pages( 1 );

    if ( arch_context->page_directory == NULL ) {
        return -ENOMEM;
    }

    memsetl( arch_context->page_directory, 0, PAGE_SIZE / 4 );

    /* Map the first 512 Mb to the kernel */

    map_region_pages( arch_context, 0, 0, 512 * 1024 * 1024 );

    region = allocate_region( "kernel" );

    if ( region == NULL ) {
        return -ENOMEM;
    }

    region->start = 0;
    region->size = 512 * 1024 * 1024;

    error = region_insert( context, region );

    if ( error < 0 ) {
        return error;
    }

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