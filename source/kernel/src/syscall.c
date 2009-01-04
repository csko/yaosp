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
#include <mm/userspace.h>

static system_call_entry_t system_call_table[] = {
    /* 0 */
    {
        .name = "fork",
        .function = sys_fork,
        .flags = SYSCALL_SAVE_STACK
    },
    /* 1 */
    {
        .name = "execve",
        .function = sys_execve,
        .flags = SYSCALL_SAVE_STACK
    },
    /* 2 */
    {
        .name = "dbprintf",
        .function = sys_dbprintf,
        .flags = 0
    },
    /* 3 */
    {
        .name = "sbrk",
        .function = sys_sbrk,
        .flags = 0
    }
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
