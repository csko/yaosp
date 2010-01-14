/* System calls
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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
#include <console.h>
#include <mm/userspace.h>
#include <mm/pages.h>
#include <sched/scheduler.h>
#include <vfs/vfs.h>
#include <network/socket.h>
#include <lock/mutex.h>

static system_call_entry_t system_call_table[] = {
    { "fork", sys_fork, SYSCALL_SAVE_STACK, PARAM_COUNT(0) },
    { "execve", sys_execve, SYSCALL_SAVE_STACK, PARAM_COUNT(1) | PARAM_TYPE(0,P_TYPE_STRING) },
    { "dbprintf", sys_dbprintf, 0, PARAM_COUNT(0) },
    { "sbrk", sys_sbrk, 0, PARAM_COUNT(0) },
    { "open", sys_open, 0, PARAM_COUNT(1) | PARAM_TYPE(0,P_TYPE_STRING) },
    { "close", sys_close, 0, PARAM_COUNT(1) | PARAM_TYPE(0,P_TYPE_INT) },
    { "read", sys_read, 0, PARAM_COUNT(0) },
    { "write", sys_write, 0, PARAM_COUNT(0) },
    { "pread", sys_pread, 0, PARAM_COUNT(0) },
    { "pwrite", sys_pwrite, 0, PARAM_COUNT(0) },
    { "dup", sys_dup, 0, PARAM_COUNT(0) },
    { "dup2", sys_dup2, 0, PARAM_COUNT(0) },
    { "isatty", sys_isatty, 0, PARAM_COUNT(1) | PARAM_TYPE(0,P_TYPE_INT) },
    { "ioctl", sys_ioctl, 0, PARAM_COUNT(0) },
    { "getdents", sys_getdents, 0, PARAM_COUNT(3) | PARAM_TYPE(0,P_TYPE_INT) | PARAM_TYPE(1,P_TYPE_PTR) | PARAM_TYPE(2,P_TYPE_UINT) },
    { "rewinddir", sys_rewinddir, 0, PARAM_COUNT(0) },
    { "chdir", sys_chdir, 0, PARAM_COUNT(0) },
    { "fchdir", sys_fchdir, 0, PARAM_COUNT(0) },
    { "stat", sys_stat, 0, PARAM_COUNT(0) },
    { "lstat", sys_lstat, 0, PARAM_COUNT(0) },
    { "fstat", sys_fstat, 0, PARAM_COUNT(2) | PARAM_TYPE(0,P_TYPE_INT) | PARAM_TYPE(1,P_TYPE_PTR) },
    { "lseek", sys_lseek, 0, PARAM_COUNT(0) },
    { "fcntl", sys_fcntl, 0, PARAM_COUNT(0) },
    { "access", sys_access, 0, PARAM_COUNT(0) },
    { "mkdir", sys_mkdir, 0, PARAM_COUNT(0) },
    { "rmdir", sys_rmdir, 0, PARAM_COUNT(0) },
    { "unlink", sys_unlink, 0, PARAM_COUNT(0) },
    { "symlink", sys_symlink, 0, PARAM_COUNT(0) },
    { "readlink", sys_readlink, 0, PARAM_COUNT(0) },
    { "mount", sys_mount, 0, PARAM_COUNT(0) },
    { "unmount", sys_unmount, 0, PARAM_COUNT(0) },
    { "select", sys_select, 0, PARAM_COUNT(0) },
    { "utime", sys_utime, 0, PARAM_COUNT(0) },
    { "exit", sys_exit, 0, PARAM_COUNT(0) },
    { "exit_thread", sys_exit_thread, 0, PARAM_COUNT(0) },
    { "wait4", sys_wait4, 0, PARAM_COUNT(0) },
    { "stime", sys_stime, 0, PARAM_COUNT(0) },
    { "get_boot_time", sys_get_boot_time, 0, PARAM_COUNT(0) },
    { "get_system_time", sys_get_system_time, 0, PARAM_COUNT(0) },
    { "get_idle_time", sys_get_idle_time, 0, PARAM_COUNT(0) },
    { "get_kernel_info", sys_get_kernel_info, 0, PARAM_COUNT(0) },
    { "get_kernel_statistics", sys_get_kernel_statistics, 0, PARAM_COUNT(0) },
    { "get_module_count", sys_get_module_count, 0, PARAM_COUNT(0) },
    { "get_module_info", sys_get_module_info, 0, PARAM_COUNT(0) },
    { "get_process_count", sys_get_process_count, 0, PARAM_COUNT(0) },
    { "get_process_info", sys_get_process_info, 0, PARAM_COUNT(0) },
    { "get_thread_count_for_process", sys_get_thread_count_for_process, 0, PARAM_COUNT(0) },
    { "get_thread_info_for_process", sys_get_thread_info_for_process, 0, PARAM_COUNT(0) },
    { "get_processor_count", sys_get_processor_count, 0, PARAM_COUNT(0) },
    { "get_processor_info", sys_get_processor_info, 0, PARAM_COUNT(0) },
    { "get_memory_info", sys_get_memory_info, 0, PARAM_COUNT(0) },
    { "sleep_thread", sys_sleep_thread, 0, PARAM_COUNT(0) },
    { "memory_region_create", sys_memory_region_create, 0, PARAM_COUNT(0) },
    { "memory_region_delete", sys_memory_region_delete, 0, PARAM_COUNT(0) },
    { "memory_region_remap_pages", sys_memory_region_remap_pages, 0, PARAM_COUNT(0) },
    { "memory_region_alloc_pages", sys_memory_region_alloc_pages, 0, PARAM_COUNT(0) },
    { "memory_region_clone_pages", sys_memory_region_clone_pages, 0, PARAM_COUNT(0) },
    { "getpid", sys_getpid, 0, PARAM_COUNT(0) },
    { "gettid", sys_gettid, 0, PARAM_COUNT(0) },
    { "load_module", sys_load_module, 0, PARAM_COUNT(0) },
    { "reboot", sys_reboot, 0, PARAM_COUNT(0) },
    { "shutdown", sys_shutdown, 0, PARAM_COUNT(0) },
    { "create_thread", sys_create_thread, 0, PARAM_COUNT(0) },
    { "wake_up_thread", sys_wake_up_thread, 0, PARAM_COUNT(0) },
    { "create_ipc_port", sys_create_ipc_port, 0, PARAM_COUNT(0) },
    { "destroy_ipc_port", sys_destroy_ipc_port, 0, PARAM_COUNT(0) },
    { "send_ipc_message", sys_send_ipc_message, 0, PARAM_COUNT(0) },
    { "recv_ipc_message", sys_recv_ipc_message, 0, PARAM_COUNT(0) },
    { "peek_ipc_message", sys_peek_ipc_message, 0, PARAM_COUNT(0) },
    { "register_named_ipc_port", sys_register_named_ipc_port, 0, PARAM_COUNT(0) },
    { "get_named_ipc_port", sys_get_named_ipc_port, 0, PARAM_COUNT(0) },
    { "sigaction", sys_sigaction, 0, PARAM_COUNT(0) },
    { "sigprocmask", sys_sigprocmask, 0, PARAM_COUNT(0) },
    { "kill", sys_kill, 0, PARAM_COUNT(0) },
    { "kill_thread", sys_kill_thread, 0, PARAM_COUNT(0) },
    { "signal_return", sys_signal_return, SYSCALL_SAVE_STACK },
    { "mutex_lock", sys_mutex_lock, 0, PARAM_COUNT(0) },
    { "mutex_trylock", sys_mutex_trylock, 0, PARAM_COUNT(0) },
    { "mutex_timedlock", sys_mutex_timedlock, 0, PARAM_COUNT(0) },
    { "mutex_unlock", sys_mutex_unlock, 0, PARAM_COUNT(0) },
    { "mutex_create", sys_mutex_create, 0, PARAM_COUNT(0) },
    { "mutex_destroy", sys_mutex_destroy, 0, PARAM_COUNT(0) },
    { "condition_wait", sys_condition_wait, 0, PARAM_COUNT(0) },
    { "condition_timedwait", sys_condition_timedwait, 0, PARAM_COUNT(0) },
    { "condition_signal", sys_condition_signal, 0, PARAM_COUNT(0) },
    { "condition_broadcast", sys_condition_broadcast, 0, PARAM_COUNT(0) },
    { "condition_create", sys_condition_create, 0, PARAM_COUNT(0) },
    { "condition_destroy", sys_condition_destroy, 0, PARAM_COUNT(0) },
    { "getrusage", sys_getrusage, 0, PARAM_COUNT(0) },
    { "socket", sys_socket, 0, PARAM_COUNT(0) },
    { "connect", sys_connect, 0, PARAM_COUNT(0) },
    { "bind", sys_bind, 0, PARAM_COUNT(0) },
    { "listen", sys_listen, 0, PARAM_COUNT(0) },
    { "accept", sys_accept, 0, PARAM_COUNT(0) },
    { "recvmsg", sys_recvmsg, 0, PARAM_COUNT(0) },
    { "sendmsg", sys_sendmsg, 0, PARAM_COUNT(0) },
    { "getsockopt", sys_getsockopt, 0, PARAM_COUNT(0) },
    { "setsockopt", sys_setsockopt, 0, PARAM_COUNT(0) },
    { "getpeername", sys_getpeername, 0, PARAM_COUNT(0) },
    { "getsockname", sys_getsockname, 0, PARAM_COUNT(0) }
};

static int trace_system_call_enter( system_call_entry_t* syscall, uint32_t* params ) {
    int i;
    int param_count;

    kprintf( INFO, "%s(", syscall->name );

    param_count = PARAM_COUNT_GET( syscall->params );

    for ( i = 0; i < param_count; i++ ) {
        int param_type = PARAM_TYPE_GET( syscall->params, i );

        switch ( param_type ) {
            case P_TYPE_INT :
                kprintf( INFO, "%d", params[ i ] );
                break;

            case P_TYPE_UINT :
                kprintf( INFO, "%u", params[ i ] );
                break;

            case P_TYPE_STRING :
                kprintf( INFO, "%s", ( char* )params[ i ] );
                break;

            case P_TYPE_PTR :
                kprintf( INFO, "%p", params[ i ] );
                break;

            default :
                kprintf( INFO, "unknown" );
                break;
        }

        if ( i != ( param_count - 1 ) ) {
            kprintf( INFO, "," );
        }
    }

    kprintf( INFO, ")" );

    return 0;
}

static int trace_system_call_exit( system_call_entry_t* syscall, int result ) {
    kprintf( INFO, "=%d\n", result );

    return 0;
}

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

    /* System call tracing */

    if ( 0 ) {
        trace_system_call_enter( syscall_entry, params );
    }

    /* Call the function associated with the system call number */

    result = syscall(
        params[ 0 ],
        params[ 1 ],
        params[ 2 ],
        params[ 3 ],
        params[ 4 ]
    );

    /* System call tracing */

    if ( 0 ) {
        trace_system_call_exit( syscall_entry, result );
    }

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
