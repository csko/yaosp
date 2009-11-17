/* i386 paging code
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

#ifndef _ARCH_MM_PAGING_H_
#define _ARCH_MM_PAGING_H_

#include <types.h>

#include <arch/mm/config.h>
#include <arch/mm/context.h>

#define PAGE_PRESENT 0x1
#define PAGE_WRITE   0x2
#define PAGE_USER    0x4

#define PGD_INDEX(addr) ((addr)>>PGDIR_SHIFT)
#define PT_INDEX(addr)  (((addr)>>PAGE_SHIFT) & 1023)

int get_paging_flags_for_region( memory_region_t* region );

int paging_alloc_table_entries( uint32_t* table, uint32_t from, uint32_t to, uint32_t flags );
int paging_fill_table_entries( uint32_t* table, uint32_t address, uint32_t from, uint32_t to, uint32_t flags );
int paging_clear_table_entries( uint32_t* table, uint32_t from, uint32_t to );
int paging_free_table_entries( uint32_t* table, uint32_t from, uint32_t to );
int paging_clone_table_entries( uint32_t* old_table, uint32_t* new_table,
    uint32_t from, uint32_t to, int remove_write );

int init_paging( void );

#endif /* _ARCH_MM_PAGING_H_ */
