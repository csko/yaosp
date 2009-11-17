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
#include <config.h>

typedef struct i386_memory_context {
    uint32_t* page_directory;
} i386_memory_context_t;

int arch_memory_context_init( memory_context_t* context );
int arch_memory_context_destroy( memory_context_t* context );

int arch_memory_context_clone( memory_context_t* old_context, memory_context_t* new_context );

#ifdef ENABLE_DEBUGGER
int arch_memory_context_translate_address( memory_context_t* context, ptr_t linear, ptr_t* physical );
#endif /* ENABLE_DEBUGGER */

#endif /* _ARCH_MM_CONTEXT_H_ */
