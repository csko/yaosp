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

int map_region_page_tables( i386_memory_context_t* arch_context, ptr_t start, uint32_t size, bool kernel ) {
    void* p;
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t flags;

    flags = PRESENT | WRITE;

    if ( !kernel ) {
        flags |= USER;
    }

    for ( addr = start; addr < ( start + size ); addr += PGDIR_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );

        if ( *pgd_entry == 0 ) {
            p = alloc_pages( 1 );

            if ( p == NULL ) {
                return -ENOMEM;
            }

            memsetl( p, 0, PAGE_SIZE / 4 );

            *pgd_entry = ( ptr_t )p | flags;
        }
    }

    return 0;
}

int map_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, ptr_t physical, uint32_t size, bool kernel ) {
    int error;
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;
    uint32_t flags;

    error = map_region_page_tables( arch_context, virtual, size, kernel );

    if ( error < 0 ) {
        return error;
    }

    flags = PRESENT | WRITE;

    if ( !kernel ) {
        flags |= USER;
    }

    for ( addr = virtual; addr < ( virtual + size ); addr += PAGE_SIZE, physical += PAGE_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );
        pt_entry = page_table_entry( *pgd_entry, addr );

        *pt_entry = physical | flags;
    }

    return 0;
}

int create_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size, bool kernel ) {
    int error;
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;
    uint32_t flags;

    error = map_region_page_tables( arch_context, virtual, size, kernel );

    if ( error < 0 ) {
        return error;
    }

    flags = PRESENT | WRITE;

    if ( !kernel ) {
        flags |= USER;
    }

    for ( addr = virtual; addr < ( virtual + size ); addr += PAGE_SIZE ) {
        void* p;

        pgd_entry = page_directory_entry( arch_context, addr );
        pt_entry = page_table_entry( *pgd_entry, addr );

        p = alloc_pages( 1 );

        if ( p == NULL ) {
            return -ENOMEM;
        }

        *pt_entry = ( ptr_t )p | flags;
    }

    return 0;
}

int free_region_page_tables( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size ) {
    int i;
    bool free;
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* page_table;

    for ( addr = virtual; addr < ( virtual + size ); addr += PGDIR_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );
        page_table = ( uint32_t* )*pgd_entry;

        free = true;

        for ( i = 0; i < 1024; i++ ) {
            if ( page_table[ i ] != 0 ) {
                free = false;
                break;
            }
        }

        if ( free ) {
            free_pages( ( void* )page_table, 1 );
            *pgd_entry = 0;
        }
    }

    return 0;
}

int free_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size ) {
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;

    for ( addr = virtual; addr < ( virtual + size ); addr += PAGE_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );
        pt_entry = page_table_entry( *pgd_entry, addr );

        free_pages( ( void* )*pt_entry, 1 );
        *pt_entry = 0;
    }

    return 0;
}

int free_region_pages_contiguous( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size ) {
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;

    pgd_entry = page_directory_entry( arch_context, virtual );
    pt_entry = page_table_entry( *pgd_entry, virtual );

    free_pages( ( void* )*pt_entry, size / PAGE_SIZE );

    for ( addr = virtual; addr < ( virtual + size ); addr += PAGE_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );
        pt_entry = page_table_entry( *pgd_entry, addr );

        *pt_entry = 0;
    }

    return 0;
}

int clone_kernel_region(
    i386_memory_context_t* old_arch_context,
    region_t* old_region,
    i386_memory_context_t* new_arch_context,
    region_t* new_region
) {
    int error;
    uint32_t addr;
    uint32_t* old_pgd_entry;
    uint32_t* old_pt_entry;
    uint32_t* new_pgd_entry;
    uint32_t* new_pt_entry;

    error = map_region_page_tables(
        new_arch_context,
        new_region->start,
        new_region->size,
        ( old_region->flags & REGION_KERNEL ) != 0
    );

    if ( error < 0 ) {
        return error;
    }

    for ( addr = old_region->start; addr < old_region->start + old_region->size; addr += PAGE_SIZE ) {
        old_pgd_entry = page_directory_entry( old_arch_context, addr );
        old_pt_entry = page_table_entry( *old_pgd_entry, addr );

        new_pgd_entry = page_directory_entry( new_arch_context, addr );
        new_pt_entry = page_table_entry( *new_pgd_entry, addr );

        *new_pt_entry = *old_pt_entry;
    }

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

    map_region_pages( arch_context, 0, 0, 512 * 1024 * 1024, true );

    region = allocate_region( "kernel" );

    if ( region == NULL ) {
        return -ENOMEM;
    }

    region->start = 0;
    region->size = 512 * 1024 * 1024;
    region->flags = REGION_READ | REGION_WRITE | REGION_KERNEL;

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
