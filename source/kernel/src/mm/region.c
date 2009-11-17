/* Memory region handling
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
#include <errno.h>
#include <smp.h>
#include <kernel.h>
#include <macros.h>
#include <console.h>
#include <lock/mutex.h>
#include <mm/region.h>
#include <mm/kmalloc.h>
#include <mm/context.h>
#include <sched/scheduler.h>
#include <lib/string.h>

#include <arch/mm/region.h>

lock_id region_lock;
hashtable_t region_table;

static int region_id_counter = 0;

memory_region_t* memory_region_allocate( const char* name ) {
    size_t name_length;
    memory_region_t* region;

    name_length = strlen( name );

    region = ( memory_region_t* )kmalloc( sizeof( memory_region_t ) + name_length + 1 );

    if ( __unlikely( region == NULL ) ) {
        return NULL;
    }

    memset( region, 0, sizeof( memory_region_t ) );

    region->name = ( char* )( region + 1 );

    memcpy( region->name, name, name_length + 1 );

    return region;
}

void memory_region_destroy( memory_region_t* region ) {
#if 0
    if ( region->file != NULL ) {
        io_context_t* io_context;

        io_context = region->context->process->io_context;
        ASSERT( io_context != NULL );

        io_context_put_file( io_context, region->file );
        region->file = NULL;
    }
#endif

    kfree( region );
}

int memory_region_insert( memory_context_t* context, memory_region_t* region ) {
    int error;

    /* Insert the new region to the hashtable */

    do {
        region->id = region_id_counter++;

        if ( region_id_counter < 0 ) {
            region_id_counter = 0;
        }
    } while ( hashtable_get( &region_table, ( void* )&region->id ) != NULL );

    error = hashtable_add( &region_table, ( hashitem_t* )region );

    if ( __unlikely( error < 0 ) ) {
        goto error1;
    }

    /* Insert it to the memory context */

    error = memory_context_insert_region( context, region );

    if ( __unlikely( error < 0 ) ) {
        goto error2;
    }

    return 0;

error2:
    hashtable_remove( &region_table, ( const void* )&region->id );

error1:
    return error;
}

int memory_region_remove( memory_context_t* context, memory_region_t* region ) {
    ASSERT( mutex_is_locked( region_lock ) );

    hashtable_remove( &region_table, ( const void* )&region->id );
    memory_context_remove_region( context, region );

    return 0;
}

memory_region_t* memory_region_get( region_id id ) {
    memory_region_t* region;

    mutex_lock( region_lock, LOCK_IGNORE_SIGNAL );

    region = ( memory_region_t* )hashtable_get( &region_table, ( const void* )&id );

    if ( region != NULL ) {
        region->ref_count++;
    }

    mutex_unlock( region_lock );

    return region;
}

int memory_region_put( memory_region_t* region ) {
    int do_delete = 0;

    mutex_lock( region_lock, LOCK_IGNORE_SIGNAL );

    if ( --region->ref_count == 0 ) {
        hashtable_remove( &region_table, ( const void* )&region->id );
        do_delete = 1;
    }

    mutex_unlock( region_lock );

    if ( do_delete ) {
        memory_context_t* context;

        context = region->context;

        mutex_lock( context->mutex, LOCK_IGNORE_SIGNAL );
        memory_context_remove_region( context, region );
        mutex_unlock( context->mutex );

        arch_memory_region_unmap_pages( region, region->address, region->size );
        memory_region_destroy( region );
    }

    return 0;
}

memory_region_t* do_create_memory_region( memory_context_t* context, const char* name,
                                          uint64_t size, uint32_t flags ) {
    ptr_t start;
    ptr_t end;
    ptr_t address;
    memory_region_t* region;

    if ( flags & REGION_KERNEL ) {
        start = FIRST_KERNEL_ADDRESS;
        end = LAST_KERNEL_ADDRESS;
    } else {
        start = FIRST_USER_ADDRESS;
        end = LAST_USER_ADDRESS;
    }

    if ( !memory_context_find_unmapped_region( context, start, end, size, &address ) ) {
        return NULL;
    }

    region = memory_region_allocate( name );

    if ( region == NULL ) {
        return NULL;
    }

    region->ref_count = 1;
    region->flags = flags;
    region->address = address;
    region->size = size;
    region->context = context;

    return region;
}

memory_region_t* do_create_memory_region_at( struct memory_context* context, const char* name,
                                             ptr_t address, uint64_t size, uint32_t flags ) {
    ptr_t dummy;
    memory_region_t* region;

    if ( !memory_context_find_unmapped_region( context, address, address + size - 1, size, &dummy ) ) {
        return NULL;
    }

    region = memory_region_allocate( name );

    if ( region == NULL ) {
        return NULL;
    }

    region->ref_count = 1;
    region->flags = flags;
    region->address = address;
    region->size = size;
    region->context = context;

    return region;
}

memory_region_t* memory_region_create( const char* name, uint64_t size, uint32_t flags ) {
    memory_region_t* region;
    memory_context_t* context;

    flags &= REGION_USER_FLAGS;

    if ( flags & REGION_KERNEL ) {
        context = &kernel_memory_context;
    } else {
        context = current_process()->memory_context;
    }

    mutex_lock( context->mutex, LOCK_IGNORE_SIGNAL );

    region = do_create_memory_region( context, name, size, flags );

    if ( region != NULL ) {
        memory_context_insert_region( context, region );
    }

    mutex_unlock( context->mutex );

    if ( region != NULL ) {
        scheduler_lock();
        context->process->vmem_size += region->size;
        scheduler_unlock();
    }

    return region;
}

int do_memory_region_remap_pages( memory_region_t* region, ptr_t physical_address ) {
    if ( region->flags & REGION_MAPPING_FLAGS ) {
        return -EINVAL;
    }

    region->flags |= REGION_REMAPPED;

    return arch_memory_region_remap_pages( region, physical_address );
}

int memory_region_remap_pages( memory_region_t* region, ptr_t physical_address ) {
    int ret;
    memory_context_t* context;

    context = region->context;

    mutex_lock( context->mutex, LOCK_IGNORE_SIGNAL );
    ret = do_memory_region_remap_pages( region, physical_address );
    mutex_unlock( context->mutex );

    return ret;
}

int do_memory_region_alloc_pages( memory_region_t* region ) {
    if ( region->flags & REGION_MAPPING_FLAGS ) {
        return -EINVAL;
    }

    region->flags |= REGION_ALLOCATED;

    return arch_memory_region_alloc_pages( region, region->address, region->size );
}

int memory_region_alloc_pages( memory_region_t* region ) {
    int ret;
    memory_context_t* context;

    context = region->context;

    mutex_lock( context->mutex, LOCK_IGNORE_SIGNAL );
    ret = do_memory_region_alloc_pages( region );
    mutex_unlock( context->mutex );

    return ret;
}

int memory_region_resize( memory_region_t* region, uint64_t new_size ) {
    int ret = 0;
    memory_context_t* context;

    context = region->context;

    mutex_lock( context->mutex, LOCK_IGNORE_SIGNAL );

    if ( new_size < region->size ) {
        arch_memory_region_unmap_pages( region, region->address + new_size, region->size - new_size );
        region->size = new_size;
    } else if ( new_size > region->size ) {
        if ( memory_context_can_resize_region( context, region, new_size ) ) {
            switch ( region->flags & REGION_MAPPING_FLAGS ) {
                case REGION_ALLOCATED :
                    ret = arch_memory_region_alloc_pages( region, region->address + region->size, new_size - region->size );
                    break;
            }

            if ( ret == 0 ) {
                region->size = new_size;
            }
        } else {
            memory_context_dump( region->context );
            ret = -ENOSPC;
        }
    }

    mutex_unlock( context->mutex );

    return ret;
}

int sys_memory_region_create( const char* name, uint64_t size, uint32_t flags ) {
    return -1;
}

int sys_memory_region_put( region_id id ) {
    return -1;
}

int sys_memory_region_map( region_id id, uint64_t* offset, ptr_t physical, uint64_t* size ) {
    return -1;
}

int sys_memory_region_clone( region_id id ) {
    return -1;
}

void memory_region_dump( memory_region_t* region, int index ) {
    kprintf(
        INFO,
        "  id: %2d region: %08p-%08p flags: ",
        region->id,
        region->address,
        region->address + region->size - 1
    );

    if ( region->flags & REGION_READ ) { kprintf( INFO, "r" ); } else { kprintf( INFO, "-" ); }
    if ( region->flags & REGION_WRITE ) { kprintf( INFO, "w" ); } else { kprintf( INFO, "-" ); }
    if ( region->flags & REGION_STACK ) { kprintf( INFO, "S" ); } else { kprintf( INFO, "-" ); }

    kprintf( INFO, " %s\n", region->name );
}

static void* region_key( hashitem_t* item ) {
    memory_region_t* region;

    region = ( memory_region_t* )item;

    return ( void* )&region->id;
}

__init int preinit_regions( void ) {
    int error;

    error = init_hashtable(
        &region_table,
        1024,
        region_key,
        hash_int,
        compare_int
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

__init int init_regions( void ) {
    region_lock = mutex_create( "region mutex", MUTEX_NONE );

    if ( region_lock < 0 ) {
        return region_lock;
    }

    return 0;
}
