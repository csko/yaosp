/* Architecture specific configurations for the memory manager
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

#ifndef _ARCH_MM_CONFIG_H_
#define _ARCH_MM_CONFIG_H_

#define PAGE_SHIFT 12
#define PAGE_SIZE  ( 1UL << PAGE_SHIFT )
#define PAGE_MASK  ( ~( PAGE_SIZE - 1 ) )
#define PAGE_ALIGN( addr )  ( ( (addr) + PAGE_SIZE - 1 ) & PAGE_MASK )

#endif // _ARCH_MM_CONFIG_H_
