/* Memory context definition
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

#ifndef _MM_CONTEXT_H_
#define _MM_CONTEXT_H_

#include <config.h>
#include <mm/region.h>
#include <lock/mutex.h>
#include <lib/array.h>

struct process;

typedef struct memory_context {
    struct memory_context* next;

    lock_id mutex;
    array_t regions;
    struct process* process;

    void* arch_data;
} memory_context_t;

extern memory_context_t kernel_memory_context;

/**
 * Inserts a previously created memory region to the specified
 * memory context. The regions in the memory context structure are
 * sorted by the address fields.
 *
 * @param context The memory context where the new region will be inserted
 * @param region The region to insert
 * @return On success 0 is returned
 */
int memory_context_insert_region( memory_context_t* context, memory_region_t* region );

/**
 * Removes a memory region from the memory context.
 *
 * @param context The memory context to remove the region from
 * @param region The region to remove
 * @return On success 0 is returned
 */
int memory_context_remove_region( memory_context_t* context, memory_region_t* region );

memory_region_t* memory_context_get_region_for( memory_context_t* context, ptr_t address );

/**
 * Searches for a size byte(s) long unmapped memory region in the specified
 * memory context.
 *
 * @param context The memory context to search in
 * @param start The start address of the range to search in
 * @param end The end address of the range to search in
 * @param size The size (in bytes) of the unmapped region to look for
 * @param address A pointer where the start address of the found memory
                  region will be stored
 * @return On success true is returned
 */
bool memory_context_find_unmapped_region( memory_context_t* context, ptr_t start, ptr_t end, uint32_t size, ptr_t* address );

/**
 * Checks if a memory context can increase its size with the specified
 * number of bytes.
 *
 * @param context The memory context
 * @param region The memory region we want to resize
 * @param new_size The new size of the memory region
 * @return True is returned if the resize is allowed
 */
bool memory_context_can_resize_region( memory_context_t* context, memory_region_t* region, uint64_t new_size );

/**
 * Clones an existing memory context.
 *
 * @param old_context The old memory context that will be cloned
 * @param new_process The process we're creating the new memory context for
 * @return The cloned memory context
 */
memory_context_t* memory_context_clone( memory_context_t* old_context, struct process* new_process );

/**
 * Deletes all regions from the specified memory context. If the
 * user_only parameter is true then the kernel regions won't be
 * deleted.
 *
 * @param context The memory context to delete regions from
 * @param user_only If true only user regions will be deleted
 * @return On success 0 is returned
 */
int memory_context_delete_regions( memory_context_t* context );

#ifdef ENABLE_DEBUGGER
int memory_context_translate_address( memory_context_t* context, ptr_t linear, ptr_t* physical );
#endif /* ENABLE_DEBUGGER */

void memory_context_dump( memory_context_t* context );

int memory_context_init( memory_context_t* context );
void memory_context_destroy( memory_context_t* context );

#endif /* _MM_CONTEXT_H_ */
