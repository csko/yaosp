/* Architecture specific configurations for the memory manager
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

#ifndef _ARCH_MM_CONFIG_H_
#define _ARCH_MM_CONFIG_H_

#include <mm/pages.h>

#define PAGE_SHIFT 12
#define PAGE_SIZE  ( 1UL << PAGE_SHIFT )
#define PAGE_MASK  ( ~( PAGE_SIZE - 1 ) )
#define PAGE_ALIGN( addr )  ( ( (addr) + PAGE_SIZE - 1 ) & PAGE_MASK )

#define PGDIR_SHIFT 22
#define PGDIR_SIZE  ( 1UL << PGDIR_SHIFT )
#define PGDIR_MASK  ( ~( PGDIR_SIZE - 1 ) )

#define FIRST_KERNEL_ADDRESS 0x100000
#define LAST_KERNEL_ADDRESS  0x3FFFFFFF

#define FIRST_USER_ADDRESS        0x40000000
#define FIRST_USER_REGION_ADDRESS 0x80000000
#define FIRST_USER_STACK_ADDRESS  0xC0000000
#define LAST_USER_ADDRESS         0xFFFFFFFF

enum arch_memory_type {
    MEM_LOW = MEM_ARCH_FIRST
};

#endif /* _ARCH_MM_CONFIG_H_ */
