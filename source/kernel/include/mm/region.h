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

typedef enum region_flags {
    REGION_READ = ( 1 << 0 ),
    REGION_WRITE = ( 1 << 1 ),
    REGION_EXECUTE = ( 1 << 2 ),
    REGION_KERNEL = ( 1 << 3 ),
    REGION_STACK = ( 1 << 4 ),
    REGION_REMAPPED = ( 1 << 5 ),
    REGION_ALLOCATED = ( 1 << 6 ),
    REGION_FILE_MAPPED = ( 1 << 7 ),
    REGION_USER_FLAGS = ( REGION_READ | REGION_WRITE | REGION_EXECUTE | REGION_KERNEL | REGION_STACK ),
    REGION_MAPPING_FLAGS = ( REGION_REMAPPED | REGION_ALLOCATED | REGION_FILE_MAPPED )
} region_flags_t;

typedef struct memory_region {
    hashitem_t hash;

    region_id id;
    char* name;
    int ref_count;

    region_flags_t flags;
    ptr_t address;
    uint64_t size;

#if 0
    struct file* file;
    off_t file_offset;
    size_t file_size;
#endif

    struct memory_context* context;
} memory_region_t;

typedef struct region_info {
    ptr_t start;
    ptr_t size;
} region_info_t;

memory_region_t* memory_region_allocate( const char* name );
void memory_region_destroy( memory_region_t* region );

int memory_region_insert( struct memory_context* context, memory_region_t* region );
int memory_region_remove( struct memory_context* context, memory_region_t* region );

memory_region_t* memory_region_get( region_id id );
int memory_region_put( memory_region_t* region );

/* Helper functions */

memory_region_t* do_create_memory_region( struct memory_context* context, const char* name,
                                          uint64_t size, uint32_t flags );
memory_region_t* do_create_memory_region_at( struct memory_context* context, const char* name,
                                             ptr_t address, uint64_t size, uint32_t flags );

int do_memory_region_remap_pages( memory_region_t* region, ptr_t physical_address );
int do_memory_region_alloc_pages( memory_region_t* region );

/* Memory region handling */

memory_region_t* memory_region_create( const char* name, uint64_t size, uint32_t flags );
int memory_region_remap_pages( memory_region_t* region, ptr_t physical_address );
int memory_region_alloc_pages( memory_region_t* region );

int memory_region_resize( memory_region_t* region, uint64_t new_size );

/* System calls */

int sys_memory_region_create( const char* name, uint64_t size, uint32_t flags );
int sys_memory_region_put( region_id id );
int sys_memory_region_map( region_id id, uint64_t* offset, ptr_t physical, uint64_t* size );
int sys_memory_region_clone( region_id id );

void memory_region_dump( memory_region_t* region, int index );

int preinit_regions( void );
int init_regions( void );

#endif /* _MM_REGION_H_ */
