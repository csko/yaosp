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

memory_region_t* memory_region_allocate( memory_context_t* context, const char* name, ptr_t address, uint64_t size, uint32_t flags ) {
    size_t name_length;
    memory_region_t* region;

    name_length = strlen( name );

    region = ( memory_region_t* )kmalloc( sizeof( memory_region_t ) + name_length + 1 );

    if ( __unlikely( region == NULL ) ) {
        return NULL;
    }

    memset( region, 0, sizeof( memory_region_t ) );

    region->id = -1;
    region->name = ( char* )( region + 1 );
    region->ref_count = 1;
    region->address = address;
    region->size = size;
    region->flags = flags;
    region->context = context;

    memcpy( region->name, name, name_length + 1 );

    return region;
}

void memory_region_destroy( memory_region_t* region ) {
    if ( region->file != NULL ) {
        io_context_t* io_context;

        io_context = region->context->process->io_context;
        ASSERT( io_context != NULL );

        io_context_put_file( io_context, region->file );
        region->file = NULL;
    }

    kfree( region );
}

static int memory_region_insert_global( memory_region_t* region ) {
    int error;

    mutex_lock( region_lock, LOCK_IGNORE_SIGNAL );

    do {
        region->id = region_id_counter++;

        if ( region_id_counter < 0 ) {
            region_id_counter = 0;
        }
    } while ( hashtable_get( &region_table, ( void* )&region->id ) != NULL );

    error = hashtable_add( &region_table, ( hashitem_t* )region );

    mutex_unlock( region_lock );

    return error;
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

static int do_memory_region_put_times( memory_region_t* region, int times ) {
    int do_delete = 0;

    mutex_lock( region_lock, LOCK_IGNORE_SIGNAL );

    ASSERT( region->ref_count >= times );
    region->ref_count -= times;

    if ( region->ref_count == 0 ) {
        if ( region->id != -1 ) {
            hashtable_remove( &region_table, ( const void* )&region->id );
        }

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

int memory_region_put( memory_region_t* region ) {
    return do_memory_region_put_times( region, 1 );
}

memory_region_t* do_create_memory_region( memory_context_t* context, const char* name,
                                          uint64_t size, uint32_t flags ) {
    ptr_t start;
    ptr_t end;
    ptr_t address;

    if ( flags & REGION_KERNEL ) {
        start = FIRST_KERNEL_ADDRESS;
        end = LAST_KERNEL_ADDRESS;
    } else {
        if ( flags & REGION_STACK ) {
            start = FIRST_USER_STACK_ADDRESS;
        } else if ( flags & REGION_CALL_FROM_USERSPACE ) {
            start = FIRST_USER_REGION_ADDRESS;
        } else {
            start = FIRST_USER_ADDRESS;
        }

        end = LAST_USER_ADDRESS;
    }

    if ( !memory_context_find_unmapped_region( context, start, end, size, &address ) ) {
        return NULL;
    }

    return memory_region_allocate( context, name, address, size, flags );
}

memory_region_t* do_create_memory_region_at( struct memory_context* context, const char* name,
                                             ptr_t address, uint64_t size, uint32_t flags ) {
    ptr_t dummy;

    if ( !memory_context_find_unmapped_region( context, address, address + size - 1, size, &dummy ) ) {
        return NULL;
    }

    return memory_region_allocate( context, name, address, size, flags );
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
    int error;

    if ( region->flags & REGION_MAPPING_FLAGS ) {
        return -EINVAL;
    }

    error = arch_memory_region_remap_pages( region, physical_address );

    if ( error == 0 ) {
        region->flags |= REGION_REMAPPED;
    }

    return error;
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
    int error;

    if ( region->flags & REGION_MAPPING_FLAGS ) {
        return -EINVAL;
    }

    error = arch_memory_region_alloc_pages( region, region->address, region->size );

    if ( error == 0 ) {
        region->flags |= REGION_ALLOCATED;
    }

    return error;
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

int memory_region_map_to_file( memory_region_t* region, int fd, off_t offset, size_t length ) {
    int ret;
    file_t* file;
    memory_context_t* context;

    context = region->context;

    file = io_context_get_file( current_process()->io_context, fd );

    if ( file == NULL ) {
        return -EINVAL;
    }

    ASSERT( atomic_get( &file->ref_count ) >= 2 );

    mutex_lock( context->mutex, LOCK_IGNORE_SIGNAL );

    if ( region->flags & REGION_MAPPING_FLAGS ) {
        ret = -EINVAL;
        goto out;
    }

    region->flags |= REGION_ALLOCATED;

    region->file = file;
    region->file_offset = offset;
    region->file_size = length;

    ret = 0;

 out:
    mutex_unlock( context->mutex );

    if ( ret != 0 ) {
        io_context_put_file( current_process()->io_context, file );
    }

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
            ret = -ENOSPC;
        }
    }

    mutex_unlock( context->mutex );

    return ret;
}

int sys_memory_region_create( const char* name, uint64_t* size, uint32_t flags, void** address ) {
    int error;
    memory_region_t* region;

    flags &= REGION_USER_FLAGS;
    flags &= ~REGION_KERNEL;

    region = memory_region_create( name, *size, flags | REGION_CALL_FROM_USERSPACE );

    if ( region == NULL ) {
        return -ENOMEM;
    }

    error = memory_region_insert_global( region );

    if ( error < 0 ) {
        memory_region_put( region );
        return error;
    }

    *address = ( void* )region->address;

    return region->id;
}

int sys_memory_region_delete( region_id id ) {
    memory_region_t* region;

    region = memory_region_get( id );

    if ( region == NULL ) {
        return -EINVAL;
    }

    return do_memory_region_put_times( region, 2 );
}

int sys_memory_region_remap_pages( region_id id, void* physical ) {
    int error;
    memory_region_t* region;

    region = memory_region_get( id );

    if ( region == NULL ) {
        return -EINVAL;
    }

    error = memory_region_remap_pages( region, ( ptr_t )physical );

    memory_region_put( region );

    return error;
}

int sys_memory_region_alloc_pages( region_id id ) {
    int error;
    memory_region_t* region;

    region = memory_region_get( id );

    if ( region == NULL ) {
        return -EINVAL;
    }

    error = memory_region_alloc_pages( region );

    memory_region_put( region );

    return error;
}

int sys_memory_region_clone_pages( region_id id, void** address ) {
    int error;
    memory_region_t* old_region;
    memory_region_t* new_region;

    old_region = memory_region_get( id );

    if ( old_region == NULL ) {
        return -EINVAL;
    }

    if ( ( old_region->flags & REGION_MAPPING_FLAGS ) != REGION_ALLOCATED ) {
        error = -EINVAL;
        goto out;
    }

    new_region = memory_region_create( old_region->name, old_region->size, old_region->flags & REGION_USER_FLAGS );

    if ( new_region == NULL ) {
        error = -ENOMEM;
        goto out;
    }

    mutex_lock( new_region->context->mutex, LOCK_IGNORE_SIGNAL );

    new_region->flags |= REGION_CLONED;
    error = arch_memory_region_clone_pages( old_region, new_region );

    mutex_unlock( new_region->context->mutex );

    if ( error != 0 ) {
        memory_region_put( new_region );
        goto out;
    }

    error = memory_region_insert_global( new_region );

    if ( error != 0 ) {
        memory_region_put( new_region );
        goto out;
    }

    error = new_region->id;
    *address = ( void* )new_region->address;

 out:
    memory_region_put( old_region );

    return error;
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

    uint32_t mapping_flags = region->flags & REGION_MAPPING_FLAGS;

    switch ( mapping_flags ) {
        case REGION_ALLOCATED : kprintf( INFO, "A" ); break;
        case REGION_REMAPPED : kprintf( INFO, "R" ); break;
        case REGION_CLONED : kprintf( INFO, "C" ); break;
        default : kprintf( INFO, "?" ); break;
    }

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

    kernel_memory_context.mutex = mutex_create( "Memory context mutex", MUTEX_NONE );

    if ( kernel_memory_context.mutex < 0 ) {
        mutex_destroy( region_lock );

        return kernel_memory_context.mutex;
    }

    return 0;
}
