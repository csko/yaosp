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
#include <process.h>
#include <scheduler.h>
#include <console.h>
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

region_t* do_memory_context_get_region_for( memory_context_t* context, ptr_t address ) {
    int i;
    region_t* region = NULL;

    for ( i = 0; i < context->region_count; i++ ) {
        if ( ( context->regions[ i ]->start <= address ) &&
             ( address < ( context->regions[ i ]->start + context->regions[ i ]->size ) ) ) {
            region = context->regions[ i ];
            break;
        }
    }

    return region;
}

region_t* memory_context_get_region_for( memory_context_t* context, ptr_t address ) {
    region_t* region;

    LOCK( region_lock );

    region = do_memory_context_get_region_for( context, address );

    UNLOCK( region_lock );

    return region;
}

bool memory_context_find_unmapped_region( memory_context_t* context, ptr_t start, ptr_t end, uint32_t size, ptr_t* address ) {
    int i;

    if ( context->region_count == 0 ) {
        if ( ( start + size - 1 ) <= end ) {
            *address = start;

            return true;
        }
    } else {
        region_t* region;

        for ( i = 0; i < context->region_count; i++ ) {
            region = context->regions[ i ];

            if ( start + size <= region->start ) {
                *address = start;

                return true;
            } else {
                start = MAX( start, region->start + region->size );
            }
        }

        if ( ( start + size - 1 ) <= end ) {
            *address = start;

            return true;
        }
    }

    return false;
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

memory_context_t* memory_context_clone( memory_context_t* old_context, process_t* new_process ) {
    int i;
    int error;
    memory_context_t* new_context;

    /* Allocate a new memory context */

    new_context = ( memory_context_t* )kmalloc( sizeof( memory_context_t ) );

    if ( new_context == NULL ) {
        return NULL;
    }

    memset( new_context, 0, sizeof( memory_context_t ) );

    new_context->process = new_process;

    /* Initialize the architecture dependent part of the memory context */

    error = arch_init_memory_context( new_context );

    if ( error < 0 ) {
        kfree( new_context );
        return NULL;
    }

    LOCK( region_lock );

    arch_clone_memory_context( old_context, new_context );

    /* Go throught regions and clone each one */

    for ( i = 0; i < old_context->region_count; i++ ) {
        region_t* region;
        region_t* new_region;

        region = old_context->regions[ i ];

        /* Don't clone kernel regions */

        if ( region->flags & REGION_KERNEL ) {
            ASSERT( region->file == NULL );

            continue;
        }

        new_region = allocate_region( region->name );

        if ( new_region == NULL ) {
            goto error;
        }

        new_region->flags = region->flags;
        new_region->alloc_method = region->alloc_method;
        new_region->start = region->start;
        new_region->size = region->size;
        new_region->context = new_context;

        /* NOTE: Here we only put the file pointer to the new region but
                 this is not fully correct because the file structures are
                 different for different processes, so later we have to
                 replace the file pointer of the new region with it's cloned
                 version from the new process after the I/O context cloning
                 is done as well. */

        new_region->file = region->file;
        new_region->file_offset = region->file_offset;
        new_region->file_size = region->file_size;

        /* Make sure the file won't be deleted until we can fix the above
           mentioned stuff. */

        if ( new_region->file != NULL ) {
            atomic_inc( &new_region->file->ref_count );
        }

        error = arch_clone_memory_region( old_context, region, new_context, new_region );

        if ( error < 0 ) {
            goto error;
        }

        error = region_insert( new_context, new_region );

        if ( error < 0 ) {
            goto error;
        }
    }

    UNLOCK( region_lock );

    return new_context;

error:
    /* TODO: cleanup! */

    UNLOCK( region_lock );

    return NULL;
}

int memory_context_fix_file_mapped_regions( memory_context_t* new_context, memory_context_t* old_context ) {
    int i;
    file_t* new_file;
    region_t* region;

    LOCK( region_lock );

    for ( i = 0; i < new_context->region_count; i++ ) {
        region = new_context->regions[ i ];

        if ( region->file == NULL ) {
            continue;
        }

        new_file = io_context_get_file( new_context->process->io_context, region->file->fd );

        if ( new_file == NULL ) {
            UNLOCK( region_lock );

            return -EBADF;
        }

        io_context_put_file( old_context->process->io_context, region->file );

        region->file = new_file;
    }

    UNLOCK( region_lock );

    return 0;
}

int memory_context_delete_regions( memory_context_t* context ) {
    int i;
    region_t* region;

    LOCK( region_lock );

    /* Delete the requested regions */

    for ( i = 0; i < context->region_count; i++ ) {
        region = context->regions[ i ];

        ASSERT( ( region->flags & REGION_KERNEL ) == 0 );

        arch_delete_region_pages( context, region );
        hashtable_remove( &region_table, ( const void* )region->id );
        destroy_region( region );

        context->regions[ i ] = NULL;
    }

    /* Set the new size of memory regions in the context */

    context->region_count = 0;

    /* Update vmem statistics */

    spinlock_disable( &scheduler_lock );

    context->process->vmem_size = 0;

    spinunlock_enable( &scheduler_lock );

    UNLOCK( region_lock );

    return 0;
}

void memory_context_destroy( memory_context_t* context ) {
    arch_destroy_memory_context( context );

    kfree( context->regions );
    kfree( context );
}

void memory_context_dump( memory_context_t* context ) {
    int i;
    region_t* region;

    kprintf( "Memory context dump:\n" );
    kprintf( "  region count: %d\n", context->region_count );

    for ( i = 0; i < context->region_count; i++ ) {
        region = context->regions[ i ];

        kprintf( "  region #%d\n", i );
        kprintf( "    id: %d name: %s\n", region->id, region->name );
        kprintf( "    start: %p size: %u\n", region->start, region->size );
        kprintf( "    flags:" );

        if ( region->flags & REGION_READ ) { kprintf( " read" ); }
        if ( region->flags & REGION_WRITE ) { kprintf( " write" ); }
        if ( region->flags & REGION_STACK ) { kprintf( " stack" ); }
        if ( region->flags & REGION_REMAPPED ) { kprintf( " remapped" ); }

        kprintf( "\n    alloc method: " );

        switch ( region->alloc_method ) {
            case ALLOC_NONE : kprintf( "none" ); break;
            case ALLOC_LAZY : kprintf( "lazy" ); break;
            case ALLOC_PAGES : kprintf( "pages" ); break;
            case ALLOC_CONTIGUOUS : kprintf( "cont" ); break;
        }

        kprintf( "\n" );
    }
}
