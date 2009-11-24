/* Memory context handling code
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
#include <console.h>
#include <mm/kmalloc.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <lib/string.h>

#include <arch/mm/context.h>
#include <arch/mm/paging.h>

int arch_memory_context_init( memory_context_t* context ) {
    i386_memory_context_t* arch_context;

    /* Create the arch. specific memory context */

    arch_context = ( i386_memory_context_t* )kmalloc( sizeof( i386_memory_context_t ) );

    if ( arch_context == NULL ) {
        return -ENOMEM;
    }

    /* Allocate a memory page for the page directory */

    arch_context->page_directory = ( uint32_t* )alloc_pages( 1, MEM_COMMON );

    if ( arch_context->page_directory == NULL ) {
        kfree( arch_context );
        return -ENOMEM;
    }

    memsetl( arch_context->page_directory, 0, PAGE_SIZE / 4 );

    /* Set the arch. pointer in the memory context */

    context->arch_data = ( void* )arch_context;

    return 0;
}

int arch_memory_context_destroy( memory_context_t* context ) {
    int i;
    uint32_t* page_directory;
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    if ( arch_context == NULL ) {
        return 0;
    }

    page_directory = arch_context->page_directory;

    for ( i = FIRST_USER_ADDRESS / PGDIR_SIZE; i < 1024; i++ ) {
        if ( page_directory[ i ] == 0 ) {
            continue;
        }

        free_pages( ( void* )( page_directory[ i ] & PAGE_MASK ), 1 );
    }

    free_pages( ( void* )page_directory, 1 );
    kfree( arch_context );

    return 0;
}

int arch_memory_context_clone( memory_context_t* old_context, memory_context_t* new_context ) {
    i386_memory_context_t* old_arch_context;
    i386_memory_context_t* new_arch_context;

    old_arch_context = ( i386_memory_context_t* )old_context->arch_data;
    new_arch_context = ( i386_memory_context_t* )new_context->arch_data;

    memcpy(
        new_arch_context->page_directory,
        old_arch_context->page_directory,
        FIRST_USER_ADDRESS / PGDIR_SIZE * sizeof( uint32_t )
    );

    return 0;
}

#ifdef ENABLE_DEBUGGER
int arch_memory_context_translate_address( memory_context_t* context, ptr_t linear, ptr_t* physical ) {
    uint32_t offset;
    uint32_t* pgd_entry;
    uint32_t* pt_entry;
    i386_memory_context_t* arch_context;

    offset = linear & ~PAGE_MASK;
    linear &= PAGE_MASK;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    pgd_entry = &arch_context->page_directory[ PGD_INDEX( linear ) ];

    if ( ( ( *pgd_entry ) & PAGE_PRESENT ) == 0 ) {
        return -EINVAL;
    }

    pt_entry = &( ( uint32_t* )( *pgd_entry & PAGE_MASK ) )[ PT_INDEX( linear ) ];

    if ( ( ( *pt_entry ) & PAGE_PRESENT ) == 0 ) {
        return -EINVAL;
    }

    *physical = ( ( *pt_entry ) & PAGE_MASK ) + offset;

    return 0;
}
#endif /* ENABLE_DEBUGGER */
