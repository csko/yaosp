/* i386 paging code
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

#ifndef _ARCH_MM_PAGING_H_
#define _ARCH_MM_PAGING_H_

#include <types.h>

#include <arch/mm/config.h>
#include <arch/mm/context.h>

#define PRESENT 0x1
#define WRITE   0x2
#define USER    0x4

static inline uint32_t* page_directory_entry( i386_memory_context_t* context, ptr_t address ) {
  return &( context->page_directory[ address >> PGDIR_SHIFT ] );
}

static inline uint32_t* page_table_entry( uint32_t table, ptr_t address ) {
  return &( ( ( uint32_t* )( table & PAGE_MASK ) )[ ( address / PAGE_SIZE ) & 1023 ] );
}

int map_region_page_tables( i386_memory_context_t* arch_context, ptr_t start, uint32_t size, bool kernel );
int map_region_pages(
    i386_memory_context_t* arch_context,
    ptr_t virtual,
    ptr_t physical,
    uint32_t size,
    bool kernel,
    bool write
);
int create_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size, bool kernel, bool write );
int free_region_page_tables( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size );
int free_region_pages( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size );
int free_region_pages_contiguous( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size );
int free_region_pages_remapped( i386_memory_context_t* arch_context, ptr_t virtual, uint32_t size );

int clone_user_region(
    i386_memory_context_t* old_arch_context,
    region_t* old_region,
    i386_memory_context_t* new_arch_context,
    region_t* new_region
);

int init_paging( void );

#endif // _ARCH_MM_PAGING_H_
