/* yaosp C library
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

#include <yaosp/thread.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

thread_id create_thread( const char* name, int priority, thread_entry_t* entry, void* arg, uint32_t user_stack_size ) {
    return syscall5(
        SYS_create_thread,
        ( int )name,
        priority,
        ( int )entry,
        ( int )arg,
        user_stack_size
    );
}

int wake_up_thread( thread_id id ) {
    return syscall1(
        SYS_wake_up_thread,
        id
    );
}
