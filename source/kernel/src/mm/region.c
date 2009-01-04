/* Memory region handling
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

#include <types.h>
#include <errno.h>
#include <smp.h>
#include <kernel.h>
#include <semaphore.h>
#include <mm/region.h>
#include <mm/kmalloc.h>
#include <mm/context.h>
#include <lib/hashtable.h>
#include <lib/string.h>

#include <arch/mm/region.h>

static int region_id_counter = 0;
static hashtable_t region_table;
static semaphore_id region_lock = -1;

region_t* allocate_region( const char* name ) {
    region_t* region;

    region = ( region_t* )kmalloc( sizeof( region_t ) );

    if ( region == NULL ) {
        return NULL;
    }

    memset( region, 0, sizeof( region_t ) );

    region->name = strdup( name );

    if ( region->name == NULL ) {
        kfree( region );
        return NULL;
    }

    return region;
}

int region_insert( memory_context_t* context, region_t* region ) {
    int error;

    /* Insert the new region to the hashtable */

    do {
        region->id = region_id_counter++;

        if ( region_id_counter < 0 ) {
            region_id_counter = 0;
        }
    } while ( hashtable_get( &region_table, ( void* )region->id ) != NULL );

    error = hashtable_add( &region_table, ( hashitem_t* )region );

    if ( error < 0 ) {
        return error;
    }

    /* Insert it to the memory context */

    error = memory_context_insert_region( context, region );

    if ( error < 0 ) {
        hashtable_remove( &region_table, ( const void* )region->id );
        return error;
    }

    return 0;
}

int region_remove( memory_context_t* context, region_t* region ) {
    hashtable_remove( &region_table, ( const void* )region->id );
    memory_context_remove_region( context, region );

    return 0;
}

region_id create_region(
    const char* name,
    uint32_t size,
    region_flags_t flags,
    alloc_type_t alloc_method,
    void** _address
) {
    int error = -1;
    bool found;
    ptr_t address;
    region_t* region;
    memory_context_t* context;

    /* Do some sanity checking */

    if ( ( size == 0 ) || ( ( size % PAGE_SIZE ) != 0 ) ) {
        return -EINVAL;
    }

    /* Decide which memory context to use */

    if ( flags & REGION_KERNEL ) {
        context = &kernel_memory_context;
    } else {
        context = current_process()->memory_context;
    }

    LOCK( region_lock );

    /* Search for a suitable unmapped memory region */

    found = memory_context_find_unmapped_region(
        context,
        size,
        ( flags & REGION_KERNEL ) != 0,
        &address
    );

    if ( !found ) {
        UNLOCK( region_lock );

        return -1;
    }

    /* Allocate a new region instance */

    region = allocate_region( name );

    if ( region == NULL ) {
        UNLOCK( region_lock );

        return -ENOMEM;
    }

    region->flags = flags;
    region->alloc_method = alloc_method;
    region->start = address;
    region->size = size;

    /* Allocate memory pages for the newly created
       region (if required) */

    switch ( alloc_method ) {
        case ALLOC_LAZY :
            panic( "Lazy page allocation not yet supported!\n" );
            error = -EINVAL;
            break;

        case ALLOC_PAGES :
        case ALLOC_CONTIGUOUS :
            error = arch_create_region_pages( context, region );
            break;

        case ALLOC_NONE :
            error = 0;
            break;
    }

    if ( error < 0 ) {
        UNLOCK( region_lock );

        return error;
    }

    /* Insert the new region to the memory context */

    error = region_insert( context, region );

    if ( error < 0 ) {
        UNLOCK( region_lock );

        return error;
    }

    *_address = ( void* )address;
    error = region->id;

    UNLOCK( region_lock );

    return error;
}

int delete_region( region_id id ) {
    region_t* region;
    memory_context_t* context;

    LOCK( region_lock );

    region = ( region_t* )hashtable_get( &region_table, ( const void* )id );

    if ( region == NULL ) {
        UNLOCK( region_lock );

        return -EINVAL;
    }

    if ( region->flags & REGION_KERNEL ) {
        context = &kernel_memory_context;
    } else {
        context = current_process()->memory_context;
    }

    arch_delete_region_pages( context, region );
    region_remove( context, region );

    UNLOCK( region_lock );

    return 0;
}

int resize_region( region_id id, uint32_t new_size ) {
    int error;
    region_t* region;
    memory_context_t* context;

    if ( ( new_size == 0 ) || ( ( new_size % PAGE_SIZE ) != 0 ) ) {
        return -EINVAL;
    }

    LOCK( region_lock );

    region = ( region_t* )hashtable_get( &region_table, ( const void* )id );

    if ( region == NULL ) {
        UNLOCK( region_lock );

        return -EINVAL;
    }

    if ( region->flags & REGION_KERNEL ) {
        context = &kernel_memory_context;
    } else {
        context = current_process()->memory_context;
    }

    if ( new_size > region->size ) {
        if ( !memory_context_can_resize_region( context, region, new_size ) ) {
            UNLOCK( region_lock );

            return -ENOMEM;
        }
    }

    error = arch_resize_region( context, region, new_size );

    if ( error < 0 ) {
        UNLOCK( region_lock );

        return error;
    }

    region->size = new_size;

    UNLOCK( region_lock );

    return 0;
}

int get_region_info( region_id id, region_info_t* info ) {
    int error;
    region_t* region;

    LOCK( region_lock );

    region = ( region_t* )hashtable_get( &region_table, ( const void* )id );

    if ( region == NULL ) {
        error = -EINVAL;
    } else {
        error = 0;

        info->start = region->start;
        info->size = region->size;
    }

    UNLOCK( region_lock );

    return error;
}

static void* region_key( hashitem_t* item ) {
    region_t* region;

    region = ( region_t* )item;

    return ( void* )region->id;
}

static uint32_t region_hash( const void* key ) {
    return ( uint32_t )key;
}

static bool region_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

int preinit_regions( void ) {
    int error;

    error = init_hashtable(
        &region_table,
        1024,
        region_key,
        region_hash,
        region_compare
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int init_regions( void ) {
    region_lock = create_semaphore( "region lock", SEMAPHORE_BINARY, 0, 1 );

    if ( region_lock < 0 ) {
        return region_lock;
    }

    return 0;
}
