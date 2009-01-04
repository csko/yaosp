/* Semaphore functions
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

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>
#include <yaosp/semaphore.h>

semaphore_id create_semaphore( const char* name, semaphore_type_t type, semaphore_flags_t flags, int count ) {
    return syscall4(
        SYS_create_semaphore,
        ( uint32_t )name,
        ( uint32_t )type,
        ( uint32_t )flags,
        count
    );
}

int delete_semaphore( semaphore_id id ) {
    return syscall1(
        SYS_delete_semaphore,
        id
    );
}

int lock_semaphore( semaphore_id id, int count, uint64_t timeout ) {
    return syscall3(
        SYS_lock_semaphore,
        id,
        count,
        ( uint32_t )&timeout
    );
}

int unlock_semaphore( semaphore_id id, int count ) {
    return syscall2(
        SYS_unlock_semaphore,
        id,
        count
    );
}
