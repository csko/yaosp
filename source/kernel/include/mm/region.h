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

#ifndef _MM_REGION_H_
#define _MM_REGION_H_

#include <types.h>
#include <lib/hashtable.h>

#include <arch/mm/config.h>

typedef int region_id;

typedef struct region {
    hashitem_t hash;

    region_id id;
    const char* name;
    ptr_t start;
    ptr_t size;
} region_t;

typedef enum alloc_type {
    ALLOC_NONE,
    ALLOC_PAGES,
    ALLOC_CONTIGUOUS
} alloc_type_t;

typedef enum region_flags {
    REGION_READ = ( 1 << 0 ),
    REGION_WRITE = ( 1 << 1 ),
    REGION_KERNEL = ( 1 << 2 )
} region_flags_t;

struct memory_context;

int arch_create_region_pages( struct memory_context* context, region_t* region, alloc_type_t alloc_method );

region_t* allocate_region( const char* name );

int region_insert( struct memory_context* context, region_t* region );

/**
 * Creates a new memory region.
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

int init_regions( void );

#endif // _MM_REGION_H_
