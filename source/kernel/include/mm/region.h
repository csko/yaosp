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

#ifndef _MM_REGION_H_
#define _MM_REGION_H_

#include <types.h>
#include <lib/hashtable.h>

#include <arch/mm/config.h>

struct memory_context;
struct file;

typedef int region_id;

typedef enum alloc_type {
    ALLOC_NONE,
    ALLOC_LAZY,
    ALLOC_PAGES,
    ALLOC_CONTIGUOUS
} alloc_type_t;

typedef enum region_flags {
    REGION_READ = ( 1 << 0 ),
    REGION_WRITE = ( 1 << 1 ),
    REGION_KERNEL = ( 1 << 2 ),
    REGION_REMAPPED = ( 1 << 3 ),
    REGION_STACK = ( 1 << 4 )
} region_flags_t;

typedef struct region {
    hashitem_t hash;

    region_id id;
    char* name;
    region_flags_t flags;
    alloc_type_t alloc_method;
    ptr_t start;
    ptr_t size;

    struct file* file;
    off_t file_offset;
    size_t file_size;

    struct memory_context* context;
} region_t;

typedef struct region_info {
    ptr_t start;
    ptr_t size;
} region_info_t;

region_t* allocate_region( const char* name );
void destroy_region( region_t* region );

int region_insert( struct memory_context* context, region_t* region );
int region_remove( struct memory_context* context, region_t* region );

region_id do_create_region(
    const char* name,
    uint32_t size,
    region_flags_t flags,
    alloc_type_t alloc_method,
    void** _address,
    bool call_from_userspace
);

/**
 * Creates a new memory region in the memory context of the
 * current process.
 *
 * @param name The name of the new memory region
 * @param size The size of the new memory region (has to be page aligned)
 * @param flags Some extra information for the region allocator
 * @param alloc_method The method how the pages in the region should be allocated
 * @param _address A pointer to a memory region where the virtual address of the
 *                 created region will be stored
 * @return On success a non-negative region ID is returned
 */
region_id create_region(
    const char* name,
    uint32_t size,
    region_flags_t flags,
    alloc_type_t alloc_method,
    void** _address
);

/**
 * Deletes a memory region.
 *
 * @param id The id of the region to delete
 * @return On success 0 is returned
 */
int delete_region( region_id id );

int do_remap_region( region_id id, ptr_t address, bool allow_kernel_region );
int remap_region( region_id id, ptr_t address );

/**
 * Resizes a memory region.
 *
 * @param id The id of the memory region to resize
 * @param new_size The new size of the memory region
 * @return On success 0 is returned
 */
int resize_region( region_id id, uint32_t new_size );

int map_region_to_file( region_id id, int fd, off_t offset, size_t length );

/**
 * Gets information about a memory region.
 *
 * @param id The id of the region to get information from
 * @param info A pointer to a region info structure to store informations
 * @return On success 0 is returned
 */
int get_region_info( region_id id, region_info_t* info );

region_id sys_create_region(
    const char* name,
    uint32_t size,
    region_flags_t flags,
    alloc_type_t alloc_method,
    void** _address
);

int sys_delete_region( region_id id );

int sys_remap_region( region_id id, ptr_t address );

int preinit_regions( void );
int init_regions( void );

#endif // _MM_REGION_H_
