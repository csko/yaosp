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

memory_region_t* allocate_region( const char* name ) {
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

void destroy_region( memory_region_t* region ) {
    if ( region->file != NULL ) {
        io_context_t* io_context;

        io_context = region->context->process->io_context;

        ASSERT( io_context != NULL );

        io_context_put_file( io_context, region->file );

        region->file = NULL;
    }

    kfree( region );
}

int region_insert( memory_context_t* context, memory_region_t* region ) {
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

int region_remove( memory_context_t* context, memory_region_t* region ) {
    //ASSERT( is_semaphore_locked( region_lock ) );

    hashtable_remove( &region_table, ( const void* )&region->id );
    memory_context_remove_region( context, region );

    return 0;
}

region_id do_create_region(
    const char* name,
    uint32_t size,
    region_flags_t flags,
    alloc_type_t alloc_method,
    void** _address,
    bool call_from_userspace
) {
    int error = -1;
    bool found;
    ptr_t address;
    memory_region_t* region;
    memory_context_t* context;

    /* Do some sanity checking */

    if ( ( size == 0 ) ||
         ( ( size % PAGE_SIZE ) != 0 ) ) {
        return -EINVAL;
    }

    /* Decide which memory context to use */

    if ( flags & REGION_KERNEL ) {
        context = &kernel_memory_context;
    } else {
        context = current_process()->memory_context;
    }

    /* Search for a suitable unmapped memory region */

    if ( ( flags & REGION_KERNEL ) != 0 ) {
        found = memory_context_find_unmapped_region(
            context,
            FIRST_KERNEL_ADDRESS,
            LAST_KERNEL_ADDRESS,
            size,
            &address
        );
    } else {
        ptr_t start_address;

        if ( ( flags & REGION_STACK ) != 0 ) {
            start_address = FIRST_USER_STACK_ADDRESS;
        } else {
            start_address = ( call_from_userspace ? FIRST_USER_REGION_ADDRESS : FIRST_USER_ADDRESS );
        }

        found = memory_context_find_unmapped_region(
            context,
            start_address,
            LAST_USER_ADDRESS,
            size,
            &address
        );
    }

    /* Not found? :( */

    if ( !found ) {
        return -ENOMEM;
    }

    /* Allocate a new region instance */

    region = allocate_region( name );

    if ( __unlikely( region == NULL ) ) {
        return -ENOMEM;
    }

    region->flags = flags;
    region->alloc_method = alloc_method;
    region->start = address;
    region->size = size;
    region->context = context;

    /* Allocate memory pages for the newly created region */

    error = arch_create_region_pages( context, region );

    if ( error < 0 ) {
        goto error1;
    }

    /* Insert the new region to the memory context */

    error = region_insert( context, region );

    if ( error < 0 ) {
        goto error2;
    }

    /* Update process vmem statistics */

    scheduler_lock();

    context->process->vmem_size += region->size;

    scheduler_unlock();

    *_address = ( void* )address;

    return region->id;

error2:
    arch_delete_region_pages( context, region );

error1:
    destroy_region( region );

    return error;
}

region_id create_region(
    const char* name,
    uint32_t size,
    region_flags_t flags,
    alloc_type_t alloc_method,
    void** _address
) {
    region_id region;

    /* Lazy page allocation is not allowed for kernel regions */

    if ( ( flags & REGION_KERNEL ) &&
         ( alloc_method == ALLOC_LAZY ) ) {
        return -EINVAL;
    }

    flags &= ( REGION_READ | REGION_WRITE | REGION_KERNEL | REGION_STACK );

    mutex_lock( region_lock );

    region = do_create_region( name, size, flags, alloc_method, _address, false );

    mutex_unlock( region_lock );

    return region;
}

region_id sys_create_region(
    const char* name,
    uint32_t size,
    region_flags_t flags,
    alloc_type_t alloc_method,
    void** _address
) {
    region_id region;

    flags &= ( REGION_READ | REGION_WRITE );

    mutex_lock( region_lock );

    region = do_create_region( name, size, flags, alloc_method, _address, true );

    mutex_unlock( region_lock );

    return region;
}

static int do_delete_region( region_id id, bool allow_kernel_region ) {
    memory_region_t* region;

    region = ( memory_region_t* )hashtable_get( &region_table, ( const void* )&id );

    if ( region == NULL ) {
        return -EINVAL;
    }

    if ( ( region->flags & REGION_KERNEL ) &&
         ( !allow_kernel_region ) ) {
        return -EPERM;
    }

    arch_delete_region_pages( region->context, region );
    region_remove( region->context, region );

    /* Update process vmem statistics */

    scheduler_lock();

    region->context->process->vmem_size -= region->size;

    scheduler_unlock();

    destroy_region( region );

    return 0;
}

int delete_region( region_id id ) {
    int error;

    mutex_lock( region_lock );
    error = do_delete_region( id, true );
    mutex_unlock( region_lock );

    return error;
}

int sys_delete_region( region_id id ) {
    int error;

    mutex_lock( region_lock );
    error = do_delete_region( id, false );
    mutex_unlock( region_lock );

    return error;
}

int do_remap_region( region_id id, ptr_t address, bool allow_kernel_region ) {
    memory_region_t* region;

    region = ( memory_region_t* )hashtable_get( &region_table, ( const void* )&id );

    if ( region == NULL ) {
        return -EINVAL;
    }

    if ( ( region->flags & REGION_KERNEL ) &&
         ( !allow_kernel_region ) ) {
        return -EPERM;
    }

    arch_delete_region_pages( region->context, region );
    arch_remap_region_pages( region->context, region, address );

    region->flags |= REGION_REMAPPED;

    return 0;
}

int remap_region( region_id id, ptr_t address ) {
    int error;

    mutex_lock( region_lock );

    error = do_remap_region( id, address, true );

    mutex_unlock( region_lock );

    return error;
}

int sys_remap_region( region_id id, ptr_t address ) {
    int error;

    mutex_lock( region_lock );

    error = do_remap_region( id, address, false );

    mutex_unlock( region_lock );

    return error;
}

int resize_region( region_id id, uint32_t new_size ) {
    int error;
    memory_region_t* region;
    memory_context_t* context;

    if ( ( new_size == 0 ) || ( ( new_size % PAGE_SIZE ) != 0 ) ) {
        return -EINVAL;
    }

    mutex_lock( region_lock );

    region = ( memory_region_t* )hashtable_get( &region_table, ( const void* )&id );

    if ( region == NULL ) {
        mutex_unlock( region_lock );

        return -EINVAL;
    }

    context = region->context;

    /* If the new size of the region is bigger than the current we
       have to check if we have free space after the region in the
       memory context. */

    if ( new_size > region->size ) {
        if ( !memory_context_can_resize_region( context, region, new_size ) ) {
            mutex_unlock( region_lock );

            return -ENOMEM;
        }
    }

    /* Let the architecture resize the region */

    error = arch_resize_region( context, region, new_size );

    if ( error < 0 ) {
        mutex_unlock( region_lock );

        return error;
    }

    /* Update process vmem statistics */

    scheduler_lock();

    context->process->vmem_size -= region->size;
    context->process->vmem_size += new_size;

    scheduler_unlock();

    region->size = new_size;

    mutex_unlock( region_lock );

    return 0;
}

int map_region_to_file( region_id id, int fd, off_t offset, size_t length ) {
    file_t* file;
    memory_region_t* region;
    io_context_t* io_context;

    io_context = current_process()->io_context;

    ASSERT( io_context != NULL );

    file = io_context_get_file( io_context, fd );

    if ( file == NULL ) {
        return -EINVAL;
    }

    ASSERT( atomic_get( &file->ref_count ) >= 2 );

    mutex_lock( region_lock );

    region = ( memory_region_t* )hashtable_get( &region_table, ( const void* )&id );

    if ( ( region == NULL ) ||
         ( region->alloc_method != ALLOC_LAZY ) ||
         ( region->file != NULL ) ) {
        mutex_unlock( region_lock );

        io_context_put_file( io_context, file );

        return -EINVAL;
    }

    region->file = file;
    region->file_offset = offset;
    region->file_size = length;

    mutex_unlock( region_lock );

    return 0;
}

int get_region_info( region_id id, region_info_t* info ) {
    int error;
    memory_region_t* region;

    mutex_lock( region_lock );

    region = ( memory_region_t* )hashtable_get( &region_table, ( const void* )&id );

    if ( region == NULL ) {
        error = -EINVAL;
    } else {
        error = 0;

        info->start = region->start;
        info->size = region->size;
    }

    mutex_unlock( region_lock );

    return error;
}

void memory_region_dump( memory_region_t* region, int index ) {
    kprintf(
        INFO,
        "  id: %2d region: %08p-%08p flags: ",
        region->id,
        region->start,
        region->start + region->size - 1
    );

    if ( region->flags & REGION_READ ) { kprintf( INFO, "r" ); } else { kprintf( INFO, "-" ); }
    if ( region->flags & REGION_WRITE ) { kprintf( INFO, "w" ); } else { kprintf( INFO, "-" ); }
    if ( region->flags & REGION_STACK ) { kprintf( INFO, "S" ); } else { kprintf( INFO, "-" ); }
    if ( region->flags & REGION_REMAPPED ) { kprintf( INFO, "R" ); } else { kprintf( INFO, "-" ); }

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
