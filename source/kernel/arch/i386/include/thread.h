/* Architecture specific thread functions
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

#ifndef _ARCH_THREAD_H_
#define _ARCH_THREAD_H_

#include <thread.h>

#include <arch/fpu.h>

enum {
    THREAD_FPU_USED = 1,
    THREAD_FPU_DIRTY = 2
};

typedef struct i386_thread {
    register_t esp;
    register_t cr2;
    uint32_t flags;
    fpu_state_t* fpu_state;

    void* signal_stack;
} i386_thread_t;

int arch_allocate_thread( thread_t* thread );
void arch_destroy_thread( thread_t* thread );

int arch_create_kernel_thread( thread_t* thread, void* entry, void* arg );
int arch_create_user_thread( thread_t* thread, void* entry, void* arg );

#endif /* _ARCH_THREAD_H_ */

