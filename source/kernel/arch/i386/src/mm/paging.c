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
#include <mm/context.h>
#include <mm/pages.h>
#include <mm/region.h>
#include <lib/string.h>

#include <arch/cpu.h>
#include <arch/mm/paging.h>

static i386_memory_context_t i386_kernel_memory_context;

extern int __ro_end;

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

int map_region_pages(
    i386_memory_context_t* arch_context,
    ptr_t virtual,
    ptr_t physical,
    uint32_t size,
    bool kernel,
    bool write
) {
    int error;
    uint32_t i;
    uint32_t flags;
    uint32_t count;
    uint32_t pd_index;
    uint32_t pt_index;
    uint32_t* pt;

    /* Create the page tables if required */

    error = map_region_page_tables( arch_context, virtual, size, kernel );

    if ( error < 0 ) {
        return error;
    }

    /* Decide which flags to use for the pages */

    flags = PRESENT;

    if ( !kernel ) {
        flags |= USER;
    }

    if ( write ) {
        flags |= WRITE;
    }

    /* Do the page mapping */

    count = size / PAGE_SIZE;
    pd_index = virtual >> PGDIR_SHIFT;
    pt_index = ( virtual >> PAGE_SHIFT ) & 1023;
    pt = ( uint32_t* )( arch_context->page_directory[ pd_index ] & PAGE_MASK );

    for ( i = 0; i < count; i++, physical += PAGE_SIZE ) {
        pt[ pt_index ] = ( uint32_t )physical | flags;

        if ( pt_index == 1023 ) {
            pt_index = 0;
            pd_index++;

            pt = ( uint32_t* )( arch_context->page_directory[ pd_index ] & PAGE_MASK );
        } else {
            pt_index++;
        }
    }

    return 0;
}

int create_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size, bool kernel, bool write ) {
    void* p;
    int error;
    uint32_t i;
    uint32_t count;
    uint32_t flags;
    uint32_t pd_index;
    uint32_t pt_index;
    uint32_t* pt;

    /* Map the page tables if required */

    error = map_region_page_tables( arch_context, virtual, size, kernel );

    if ( error < 0 ) {
        return error;
    }

    /* Decide what flags to use for the pages */

    flags = PRESENT;

    if ( !kernel ) {
        flags |= USER;
    }

    if ( write ) {
        flags |= WRITE;
    }

    count = size / PAGE_SIZE;
    pd_index = virtual >> PGDIR_SHIFT;
    pt_index = ( virtual >> PAGE_SHIFT ) & 1023;
    pt = ( uint32_t* )( arch_context->page_directory[ pd_index ] & PAGE_MASK );

    for ( i = 0; i < count; i++ ) {
        p = alloc_pages( 1 );

        if ( p == NULL ) {
            return -ENOMEM;
        }

        pt[ pt_index ] = ( uint32_t )p | flags;

        if ( pt_index == 1023 ) {
            pt_index = 0;
            pd_index++;

            pt = ( uint32_t* )( arch_context->page_directory[ pd_index ] & PAGE_MASK );
        } else {
            pt_index++;
        }
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
        page_table = ( uint32_t* )( *pgd_entry & PAGE_MASK );

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

        free_pages( ( void* )( *pt_entry & PAGE_MASK ), 1 );

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

    free_pages( ( void* )( *pt_entry & PAGE_MASK ), size / PAGE_SIZE );

    for ( addr = virtual; addr < ( virtual + size ); addr += PAGE_SIZE ) {
        pgd_entry = page_directory_entry( arch_context, addr );
        pt_entry = page_table_entry( *pgd_entry, addr );

        *pt_entry = 0;
    }

    return 0;
}

int free_region_pages_remapped( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size ) {
    ptr_t addr;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;

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
    uint32_t i;
    uint32_t count;
    uint32_t pt_index;
    uint32_t pd_index;
    register uint32_t* old_pt;
    register uint32_t* new_pt;

    error = map_region_page_tables(
        new_arch_context,
        new_region->start,
        new_region->size,
        true
    );

    if ( error < 0 ) {
        return error;
    }

    count = old_region->size / PAGE_SIZE;
    pd_index = old_region->start >> PGDIR_SHIFT;
    pt_index = ( old_region->start >> PAGE_SHIFT ) & 1023;
    old_pt = ( uint32_t* )( old_arch_context->page_directory[ pd_index ] & PAGE_MASK );
    new_pt = ( uint32_t* )( new_arch_context->page_directory[ pd_index ] & PAGE_MASK );

    for ( i = 0; i < count; i++ ) {
        new_pt[ pt_index ] = old_pt[ pt_index ];

        if ( pt_index == 1023 ) {
            pt_index = 0;
            pd_index++;

            old_pt = ( uint32_t* )( old_arch_context->page_directory[ pd_index ] & PAGE_MASK );
            new_pt = ( uint32_t* )( new_arch_context->page_directory[ pd_index ] & PAGE_MASK );
        } else {
            pt_index++;
        }
    }

    new_region->flags |= REGION_REMAPPED;

    return 0;
}

static int clone_user_region_pages( 
    i386_memory_context_t* old_arch_context,
    region_t* old_region,
    i386_memory_context_t* new_arch_context,
    region_t* new_region
) {
    void* p;
    uint32_t i;
    uint32_t count;
    uint32_t pt_index;
    uint32_t pd_index;
    uint32_t old_pt_entry;
    register uint32_t* old_pt;
    register uint32_t* new_pt;

    count = old_region->size / PAGE_SIZE;
    pd_index = old_region->start >> PGDIR_SHIFT;
    pt_index = ( old_region->start >> PAGE_SHIFT ) & 1023;
    old_pt = ( uint32_t* )( old_arch_context->page_directory[ pd_index ] & PAGE_MASK );
    new_pt = ( uint32_t* )( new_arch_context->page_directory[ pd_index ] & PAGE_MASK );

    for ( i = 0; i < count; i++ ) {
        p = alloc_pages( 1 );

        if ( p == NULL ) {
            return -ENOMEM;
        }

        old_pt_entry = old_pt[ pt_index ];

        memcpy( p, ( void* )( old_pt_entry & PAGE_MASK ), PAGE_SIZE );

        new_pt[ pt_index ] = ( uint32_t )p | ( old_pt_entry & ~PAGE_MASK );

        if ( pt_index == 1023 ) {
            pt_index = 0;
            pd_index++;

            old_pt = ( uint32_t* )( old_arch_context->page_directory[ pd_index ] & PAGE_MASK );
            new_pt = ( uint32_t* )( new_arch_context->page_directory[ pd_index ] & PAGE_MASK );
        } else {
            pt_index++;
        }
    }

    return 0;
}

static int clone_user_region_contiguous( 
    i386_memory_context_t* old_arch_context,
    region_t* old_region,
    i386_memory_context_t* new_arch_context,
    region_t* new_region
) {
    int i;
    void* p;
    uint8_t* tmp;
    uint32_t count;
    uint32_t pt_index;
    uint32_t pd_index;
    register uint32_t* old_pt;
    register uint32_t* new_pt;

    count = old_region->size / PAGE_SIZE;

    p = alloc_pages( count );

    if ( p == NULL ) {
        return -ENOMEM;
    }

    tmp = ( uint8_t* )p;
    pd_index = old_region->start >> PGDIR_SHIFT;
    pt_index = ( old_region->start >> PAGE_SHIFT ) & 1023;
    old_pt = ( uint32_t* )( old_arch_context->page_directory[ pd_index ] & PAGE_MASK );
    new_pt = ( uint32_t* )( new_arch_context->page_directory[ pd_index ] & PAGE_MASK );

    memcpy( p, ( void* )( old_pt[ pt_index ] & PAGE_MASK ), old_region->size );

    for ( i = 0; i < count; i++, tmp += PAGE_SIZE ) {
        new_pt[ pt_index ] = ( uint32_t )tmp | ( old_pt[ pt_index ] & ~PAGE_MASK );

        if ( pt_index == 1023 ) {
            pt_index = 0;
            pd_index++;

            old_pt = ( uint32_t* )( old_arch_context->page_directory[ pd_index ] & PAGE_MASK );
            new_pt = ( uint32_t* )( new_arch_context->page_directory[ pd_index ] & PAGE_MASK );
        } else {
            pt_index++;
        }
    }

    return 0;
}

int clone_user_region(
    i386_memory_context_t* old_arch_context,
    region_t* old_region,
    i386_memory_context_t* new_arch_context,
    region_t* new_region
) {
    int error;

    error = map_region_page_tables(
        new_arch_context,
        new_region->start,
        new_region->size,
        false
    );

    if ( error < 0 ) {
        return error;
    }

    switch ( ( int )old_region->alloc_method ) {
        case ALLOC_PAGES :
            error = clone_user_region_pages( old_arch_context, old_region, new_arch_context, new_region );

            if ( error < 0 ) {
                return error;
            }

            break;

        case ALLOC_CONTIGUOUS : {
            error = clone_user_region_contiguous( old_arch_context, old_region, new_arch_context, new_region );

            if ( error < 0 ) {
                return error;
            }

            break;
        }

        default :
            panic( "Not yet implemented!\n" );
            break;
    }

    return 0;
}

static int create_initial_region( const char* name, uint32_t start, uint32_t size, bool writable ) {
    int error;
    region_t* region;

    region = allocate_region( name );

    if ( region == NULL ) {
        return -ENOMEM;
    }

    region->start = start;
    region->size = size;
    region->flags = REGION_READ | REGION_KERNEL | REGION_REMAPPED;
    region->alloc_method = ALLOC_CONTIGUOUS;
    region->context = &kernel_memory_context;

    if ( writable ) {
        region->flags |= REGION_WRITE;
    }

    error = region_insert( &kernel_memory_context, region );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int init_paging( void ) {
    int error;
    uint32_t ro_size;
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

    ro_size = ( uint32_t )&__ro_end - 0x100000;
    ASSERT( ( ro_size % PAGE_SIZE ) == 0 );

    map_region_pages(
        arch_context,
        0,
        0,
        1 * 1024 * 1024,
        true,
        true
    );
    map_region_pages(
        arch_context,
        1 * 1024 * 1024,
        1 * 1024 * 1024,
        ro_size,
        true,
        false
    );
    map_region_pages(
        arch_context,
        1 * 1024 * 1024 + ro_size,
        1 * 1024 * 1024 + ro_size,
        512 * 1024 * 1024 - ( 1 * 1024 * 1024 + ro_size ),
        true,
        true
    );

    error = create_initial_region( "1mb", 0, 1 * 1024 * 1024, true );

    if ( error < 0 ) {
        return error;
    }

    error = create_initial_region( "kernel_ro", 1 * 1024 * 1024, ro_size, false );

    if ( error < 0 ) {
        return error;
    }

    error = create_initial_region(
        "kernel_rw",
        1 * 1024 * 1024 + ro_size,
        512 * 1024 * 1024 - ( 1 * 1024 * 1024 + ro_size ),
        true
    );

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
