/* Memory context definition
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

#ifndef _MM_CONTEXT_H_
#define _MM_CONTEXT_H_

#include <mm/region.h>

typedef struct memory_context {
    struct memory_context* next;

    int max_regions;
    int region_count;
    region_t** regions;

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
int memory_context_insert_region( memory_context_t* context, region_t* region );

/**
 * Searches for a size byte(s) long unmapped memory region in the specified
 * memory context.
 *
 * @param context The memory context to search in
 * @param size The size (in bytes) of the unmapped region to look for
 * @param address A pointer where the start address of the found memory
                  region will be stored
 * @return On success true is returned
 */
bool memory_context_find_unmapped_region( memory_context_t* context, uint32_t size, bool kernel_region, ptr_t* address );

#endif // _MM_CONTEXT_H_
