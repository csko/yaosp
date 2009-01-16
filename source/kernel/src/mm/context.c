/* Memory context handling code
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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
#include <macros.h>
#include <kernel.h>
#include <semaphore.h>
#include <mm/context.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/mm/config.h>
#include <arch/mm/context.h>
#include <arch/mm/region.h>

#define REGION_STEP_SIZE 32

memory_context_t kernel_memory_context;

extern semaphore_id region_lock;
extern hashtable_t region_table;

int memory_context_insert_region( memory_context_t* context, region_t* region ) {
    int i;
    int regions_to_move;

    /* First check if we have free space for the new region */

    if ( ( context->region_count + 1 ) >= context->max_regions ) {
        int new_max;
        region_t** new_regions;

        new_max = context->max_regions + REGION_STEP_SIZE;
        new_regions = ( region_t** )kmalloc( sizeof( region_t* ) * new_max );

        if ( new_regions == NULL ) {
            return -ENOMEM;
        }

        if ( context->region_count > 0 ) {
            memcpy( new_regions, context->regions, sizeof( region_t* ) * context->region_count );
        }

        kfree( context->regions );

        context->regions = new_regions;
        context->max_regions = new_max;
    }

    /* Insert the region to the list */

    for ( i = 0; i < context->region_count; i++ ) {
        if ( region->start < context->regions[ i ]->start ) {
            break;
        }
    }

    regions_to_move = context->region_count - i;

    if ( regions_to_move > 0 ) {
        memmove( &context->regions[ i + 1 ], &context->regions[ i ], sizeof( region_t* ) * regions_to_move );
    }

    context->regions[ i ] = region;
    context->region_count++;

    return 0;
}

int memory_context_remove_region( memory_context_t* context, region_t* region ) {
    int i;
    int regions_to_move;
    bool found = false;

    for ( i = 0; i < context->region_count; i++ ) {
        if ( context->regions[ i ] == region ) {
            found = true;
            break;
        }
    }

    if ( !found ) {
        return -EINVAL;
    }

    regions_to_move = context->region_count - i;

    if ( regions_to_move > 0 ) {
        memmove( &context->regions[ i ], &context->regions[ i + 1 ], sizeof( region_t* ) * regions_to_move );
    }

    context->regions[ context->region_count - 1 ] = NULL;
    context->region_count--;

    return 0;
}

bool memory_context_find_unmapped_region( memory_context_t* context, uint32_t size, bool kernel_region, ptr_t* address ) {
    int i;
    ptr_t start;
    ptr_t end;

    if ( kernel_region ) {
        start = FIRST_KERNEL_ADDRESS;
        end = LAST_KERNEL_ADDRESS;
    } else {
        start = FIRST_USER_ADDRESS;
        end = LAST_USER_ADDRESS;
    }

    /* Check free space before the first region */

    if ( ( context->region_count > 0 ) &&
         ( start < context->regions[ 0 ]->start ) ) {
        ptr_t tmp_end;
        ptr_t free_size;

        tmp_end = MIN( end, context->regions[ 0 ]->start - 1 );
        free_size = tmp_end - start + 1;

        if ( free_size >= size ) {
            *address = start;

            return true;
        }
    }

    for ( i = 0; i < context->region_count; i++ ) {
        region_t* region;

        region = context->regions[ i ];

        if ( start < region->start ) {
            /* The start address for the search is before the current region */

            break;
        }

        if ( ( start >= region->start ) && ( start < region->start + region->size ) ) {
            /* The start address is inside of the current region */

            break;
        }
    }

    for ( ; i < context->region_count - 1; i++ ) {
        ptr_t tmp_end;
        ptr_t free_size;

        tmp_end = MIN( end, context->regions[ i + 1 ]->start );
        free_size = tmp_end - ( context->regions[ i ]->start + context->regions[ i ]->size );

        if ( free_size >= size ) {
            *address = context->regions[ i ]->start + context->regions[ i ]->size;

            return true;
        }
    }

    ptr_t remaining_size;

    if ( context->regions > 0 ) {
        region_t* last_region;

        last_region = context->regions[ context->region_count - 1 ];

        start = MAX( start, last_region->start + last_region->size );
    }

    if ( start >= end ) {
        return false;
    }

    remaining_size = end - start;

    if ( remaining_size < size ) {
        return false;
    }

    *address = start;

    return true;
}

bool memory_context_can_resize_region( memory_context_t* context, region_t* region, uint32_t new_size ) {
    int i;
    bool found = false;

    for ( i = 0; i < context->region_count; i++ ) {
        if ( context->regions[ i ] == region ) {
            found = true;
            break;
        }
    }

    if ( !found ) {
        return false;
    }

    if ( i == context->region_count - 1 ) {
        ptr_t end_address;

        if ( region->flags & REGION_KERNEL ) {
            end_address = LAST_KERNEL_ADDRESS;
        } else {
            end_address = LAST_USER_ADDRESS;
        }

        if ( ( region->start + new_size - 1 ) > end_address ) {
            return false;
        }

        return true;
    }

    return ( ( region->start + new_size - 1 ) < context->regions[ i + 1 ]->start );
}

memory_context_t* memory_context_clone( memory_context_t* old_context ) {
    int i;
    int error;
    memory_context_t* new_context;

    /* Allocate a new memory context */

    new_context = ( memory_context_t* )kmalloc( sizeof( memory_context_t ) );

    if ( new_context == NULL ) {
        return NULL;
    }

    memset( new_context, 0, sizeof( memory_context_t ) );

    /* Initialize the architecture dependent part of the memory context */

    error = arch_init_memory_context( new_context );

    if ( error < 0 ) {
        kfree( new_context );
        return NULL;
    }

    /* Go throught regions and clone each one */

    for ( i = 0; i < old_context->region_count; i++ ) {
        region_t* region;
        region_t* new_region;

        region = old_context->regions[ i ];

        new_region = ( region_t* )kmalloc( sizeof( region_t ) );

        if ( new_region == NULL ) {
            return NULL;
        }

        new_region->name = strdup( region->name );

        if ( new_region->name == NULL ) {
            kfree( new_region );
            return NULL;
        }

        new_region->flags = region->flags;
        new_region->start = region->start;
        new_region->size = region->size;

        error = arch_clone_memory_region( old_context, region, new_context, new_region );

        if ( error < 0 ) {
            return NULL;
        }

        error = region_insert( new_context, new_region );

        if ( error < 0 ) {
            return NULL;
        }
    }
  
    return new_context;
}

int memory_context_delete_regions( memory_context_t* context, bool user_only ) {
    int i;
    int new_size;
    region_t* region;

    i = 0;

    LOCK( region_lock );

    /* If we have to delete the user regions only first we skip
       the kernel regions. */

    if ( user_only ) {
        for ( ; i < context->region_count; i++ ) {
            if ( ( context->regions[ i ]->flags & REGION_KERNEL ) == 0 ) {
                break;
            }
        }
    }

    new_size = i;

    /* Delete the requested regions */

    for ( ; i < context->region_count; i++ ) {
        region = context->regions[ i ];

        arch_delete_region_pages( context, region );
        hashtable_remove( &region_table, ( const void* )region->id );
        destroy_region( region );

        context->regions[ i ] = NULL;
    }

    /* Set the new size of memory regions in the context */

    context->region_count = new_size;

    UNLOCK( region_lock );

    return 0;
}
