/* Memory context handling code
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

#ifndef _ARCH_MM_CONTEXT_H_
#define _ARCH_MM_CONTEXT_H_

#include <types.h>

typedef struct i386_memory_context {
    uint32_t* page_directory;
} i386_memory_context_t;

int arch_init_memory_context( memory_context_t* context );

int arch_clone_memory_region(
    memory_context_t* old_context,
    region_t* old_region,
    memory_context_t* new_context,
    region_t* new_region
);

#endif // _ARCH_MM_CONTEXT_H_
