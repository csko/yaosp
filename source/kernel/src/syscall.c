/* System calls
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

#include <syscall.h>
#include <macros.h>
#include <errno.h>
#include <fork.h>
#include <smp.h>
#include <loader.h>
#include <kernel.h>
#include <semaphore.h>
#include <mm/userspace.h>
#include <vfs/vfs.h>
#include <time.h>

static system_call_entry_t system_call_table[] = {
    { "fork", sys_fork, SYSCALL_SAVE_STACK },
    { "execve", sys_execve, SYSCALL_SAVE_STACK },
    { "dbprintf", sys_dbprintf, 0 },
    { "sbrk", sys_sbrk, 0 },
    { "create_semaphore", sys_create_semaphore, 0 },
    { "delete_semaphore", sys_delete_semaphore, 0 },
    { "lock_semaphore", sys_lock_semaphore, 0 },
    { "unlock_semaphore", sys_unlock_semaphore, 0 },
    { "open", sys_open, 0 },
    { "close", sys_close, 0 },
    { "read", sys_read, 0 },
    { "write", sys_write, 0 },
    { "dup2", sys_dup2, 0 },
    { "isatty", sys_isatty, 0 },
    { "getdents", sys_getdents, 0 },
    { "fchdir", sys_fchdir, 0 },
    { "stat", sys_stat, 0 },
    { "exit", sys_exit, 0 },
    { "waitpid", sys_waitpid, 0 },
    { "time", sys_time, 0 }
};

int handle_system_call( uint32_t number, uint32_t* parameters, void* stack ) {
    system_call_t* syscall;
    system_call_entry_t* syscall_entry;

    /* Make sure this is a valid system call */

    if ( number >= ARRAY_SIZE( system_call_table ) ) {
        return -EINVAL;
    }

    syscall_entry = &system_call_table[ number ];

    /* Save the stack after the system call */

    if ( syscall_entry->flags & SYSCALL_SAVE_STACK ) {
        current_thread()->syscall_stack = stack;
    }

    syscall = ( system_call_t* )syscall_entry->function;

    /* Call the function associated with the system call number */

    return syscall(
        parameters[ 0 ],
        parameters[ 1 ],
        parameters[ 2 ],
        parameters[ 3 ],
        parameters[ 4 ]
    );
}
