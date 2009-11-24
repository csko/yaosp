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
#include <time.h>
#include <thread.h>
#include <module.h>
#include <ipc.h>
#include <syscall.h>
#include <mm/userspace.h>
#include <mm/pages.h>
#include <sched/scheduler.h>
#include <vfs/vfs.h>
#include <network/socket.h>
#include <lock/mutex.h>

#include <arch/pit.h>

static system_call_entry_t system_call_table[] = {
    { "fork", sys_fork, SYSCALL_SAVE_STACK },
    { "execve", sys_execve, SYSCALL_SAVE_STACK },
    { "dbprintf", sys_dbprintf, 0 },
    { "sbrk", sys_sbrk, 0 },
    { "open", sys_open, 0 },
    { "close", sys_close, 0 },
    { "read", sys_read, 0 },
    { "write", sys_write, 0 },
    { "pread", sys_pread, 0 },
    { "pwrite", sys_pwrite, 0 },
    { "dup", sys_dup, 0 },
    { "dup2", sys_dup2, 0 },
    { "isatty", sys_isatty, 0 },
    { "ioctl", sys_ioctl, 0 },
    { "getdents", sys_getdents, 0 },
    { "rewinddir", sys_rewinddir, 0 },
    { "chdir", sys_chdir, 0 },
    { "fchdir", sys_fchdir, 0 },
    { "stat", sys_stat, 0 },
    { "lstat", sys_lstat, 0 },
    { "fstat", sys_fstat, 0 },
    { "lseek", sys_lseek, 0 },
    { "fcntl", sys_fcntl, 0 },
    { "access", sys_access, 0 },
    { "mkdir", sys_mkdir, 0 },
    { "rmdir", sys_rmdir, 0 },
    { "unlink", sys_unlink, 0 },
    { "symlink", sys_symlink, 0 },
    { "readlink", sys_readlink, 0 },
    { "mount", sys_mount, 0 },
    { "unmount", sys_unmount, 0 },
    { "select", sys_select, 0 },
    { "utime", sys_utime, 0 },
    { "exit", sys_exit, 0 },
    { "exit_thread", sys_exit_thread, 0 },
    { "wait4", sys_wait4, 0 },
    { "stime", sys_stime, 0 },
    { "get_boot_time", sys_get_boot_time, 0 },
    { "get_system_time", sys_get_system_time, 0 },
    { "get_idle_time", sys_get_idle_time, 0 },
    { "get_kernel_info", sys_get_kernel_info, 0 },
    { "get_kernel_statistics", sys_get_kernel_statistics, 0 },
    { "get_module_count", sys_get_module_count, 0 },
    { "get_module_info", sys_get_module_info, 0 },
    { "get_process_count", sys_get_process_count, 0 },
    { "get_process_info", sys_get_process_info, 0 },
    { "get_thread_count_for_process", sys_get_thread_count_for_process, 0 },
    { "get_thread_info_for_process", sys_get_thread_info_for_process, 0 },
    { "get_processor_count", sys_get_processor_count, 0 },
    { "get_processor_info", sys_get_processor_info, 0 },
    { "get_memory_info", sys_get_memory_info, 0 },
    { "sleep_thread", sys_sleep_thread, 0 },
    { "memory_region_create", sys_memory_region_create, 0 },
    { "memory_region_delete", sys_memory_region_delete, 0 },
    { "memory_region_remap_pages", sys_memory_region_remap_pages, 0 },
    { "memory_region_alloc_pages", sys_memory_region_alloc_pages, 0 },
    { "memory_region_clone_pages", sys_memory_region_clone_pages, 0 },
    { "getpid", sys_getpid, 0 },
    { "gettid", sys_gettid, 0 },
    { "load_module", sys_load_module, 0 },
    { "reboot", sys_reboot, 0 },
    { "shutdown", sys_shutdown, 0 },
    { "create_thread", sys_create_thread, 0 },
    { "wake_up_thread", sys_wake_up_thread, 0 },
    { "create_ipc_port", sys_create_ipc_port, 0 },
    { "destroy_ipc_port", sys_destroy_ipc_port, 0 },
    { "send_ipc_message", sys_send_ipc_message, 0 },
    { "recv_ipc_message", sys_recv_ipc_message, 0 },
    { "peek_ipc_message", sys_peek_ipc_message, 0 },
    { "register_named_ipc_port", sys_register_named_ipc_port, 0 },
    { "get_named_ipc_port", sys_get_named_ipc_port, 0 },
    { "sigaction", sys_sigaction, 0 },
    { "sigprocmask", sys_sigprocmask, 0 },
    { "kill", sys_kill, 0 },
    { "kill_thread", sys_kill_thread, 0 },
    { "signal_return", sys_signal_return, SYSCALL_SAVE_STACK },
    { "mutex_lock", sys_mutex_lock, 0 },
    { "mutex_trylock", sys_mutex_trylock, 0 },
    { "mutex_timedlock", sys_mutex_timedlock, 0 },
    { "mutex_unlock", sys_mutex_unlock, 0 },
    { "mutex_create", sys_mutex_create, 0 },
    { "mutex_destroy", sys_mutex_destroy, 0 },
    { "condition_wait", sys_condition_wait, 0 },
    { "condition_timedwait", sys_condition_timedwait, 0 },
    { "condition_signal", sys_condition_signal, 0 },
    { "condition_broadcast", sys_condition_broadcast, 0 },
    { "condition_create", sys_condition_create, 0 },
    { "condition_destroy", sys_condition_destroy, 0 },
    { "getrusage", sys_getrusage, 0 }
};

int handle_system_call( uint32_t number, uint32_t* params, void* stack ) {
    int result;
    uint64_t now;
    thread_t* thread;
    system_call_t* syscall;
    system_call_entry_t* syscall_entry;

    /* Make sure this is a valid system call */

    if ( __unlikely( number >= ARRAY_SIZE( system_call_table ) ) ) {
        return -EINVAL;
    }

    thread = current_thread();
    syscall_entry = &system_call_table[ number ];

    /* Update timing information of the thread */

    now = get_system_time();

    scheduler_lock();
    thread->in_system = 1;
    thread->user_time += ( now - thread->prev_checkpoint );
    thread->prev_checkpoint = now;
    scheduler_unlock();

    /* Save the stack after the system call */

    if ( __unlikely( syscall_entry->flags & SYSCALL_SAVE_STACK ) ) {
        thread->syscall_stack = stack;
    }

    syscall = ( system_call_t* )syscall_entry->function;

    /* Call the function associated with the system call number */

    result = syscall(
        params[ 0 ],
        params[ 1 ],
        params[ 2 ],
        params[ 3 ],
        params[ 4 ]
    );

    /* Update timing information again :) */

    ASSERT( thread->in_system );

    now = get_system_time();

    scheduler_lock();
    thread->in_system = 0;
    thread->sys_time += ( now - thread->prev_checkpoint );
    thread->prev_checkpoint = now;
    scheduler_unlock();

    return result;
}
