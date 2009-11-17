/* Memory region handling
 *
 * Copyright (c) 2009 Zoltan Kovacs
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

#ifndef _ARCH_MM_REGION_H_
#define _ARCH_MM_REGION_H_

#include <mm/context.h>
#include <mm/region.h>

int arch_memory_region_remap_pages( memory_region_t* region, ptr_t physical_address );
int arch_memory_region_alloc_pages( memory_region_t* region, ptr_t virtual, uint64_t size );
int arch_memory_region_unmap_pages( memory_region_t* region, ptr_t virtual, uint64_t size );

int arch_memory_region_clone_pages( memory_region_t* old_region, memory_region_t* new_region );

#endif /* _ARCH_MM_REGION_H_ */
