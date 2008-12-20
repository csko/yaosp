/* Architecture specific thread functions
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

#ifndef _ARCH_THREAD_H_
#define _ARCH_THREAD_H_

#include <thread.h>

typedef struct i386_thread {
    register_t esp;
} i386_thread_t;

int arch_allocate_thread( thread_t* thread );
int arch_create_kernel_thread( thread_t* thread, void* entry, void* arg );

#endif // _ARCH_THREAD_H_
