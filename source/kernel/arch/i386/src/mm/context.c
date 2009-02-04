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
#include <mm/kmalloc.h>
#include <mm/context.h>
#include <mm/pages.h>
#include <lib/string.h>

#include <arch/mm/context.h>
#include <arch/mm/paging.h>

int arch_init_memory_context( memory_context_t* context ) {
    i386_memory_context_t* arch_context;

    /* Create the arch. specific memory context */

    arch_context = ( i386_memory_context_t* )kmalloc( sizeof( i386_memory_context_t ) );

    if ( arch_context == NULL ) {
        return -ENOMEM;
    }

    /* Allocate a memory page for the page directory */

    arch_context->page_directory = ( uint32_t* )alloc_pages( 1 );

    if ( arch_context->page_directory == NULL ) {
        kfree( arch_context );
        return -ENOMEM;
    }

    memsetl( arch_context->page_directory, 0, PAGE_SIZE / 4 );

    /* Set the arch. pointer in the memory context */

    context->arch_data = ( void* )arch_context;

    return 0;
}

int arch_destroy_memory_context( memory_context_t* context ) {
    i386_memory_context_t* arch_context;

    arch_context = ( i386_memory_context_t* )context->arch_data;

    free_pages( ( void* )arch_context->page_directory, 1 );
    kfree( arch_context );

    return 0;
}

int arch_clone_memory_context( memory_context_t* old_context, memory_context_t* new_context ) {
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
