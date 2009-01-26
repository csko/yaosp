/* System information definitions
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

#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#include <types.h>
#include <process.h>

typedef enum proc_listener_event {
    PROCESS_CREATED,
    PROCESS_DESTROYED,
    PROCESS_RENAMED,
    THREAD_CREATED,
    THREAD_DESTROYED,
    THREAD_RENAMED,
} proc_listener_event_t;

typedef struct process_listener {
    int ( *process_created )( process_t* process );
    int ( *process_destroyed )( process_id id );
    int ( *process_renamed )( process_t* process );
    int ( *thread_created )( thread_t* thread );
    int ( *thread_destroyed )( process_id proc_id, thread_id thr_id );
    int ( *thread_renamed )( thread_t* thread );
} process_listener_t;

typedef struct system_info {
    /* Memory information */

    uint32_t free_page_count;
    uint32_t total_page_count;

    /* Process & thread information */

    uint32_t process_count;
    uint32_t thread_count;

    /* Processor information */

    uint32_t total_processor_count;
    uint32_t active_processor_count;

    /* Module informations */

    uint32_t loaded_module_count;
} system_info_t;

int set_process_listener( process_listener_t* listener );
int notify_process_listener( proc_listener_event_t event, process_t* process, thread_t* thread );

int sys_get_system_info( system_info_t* system_info );

#endif // _SYSINFO_H_
