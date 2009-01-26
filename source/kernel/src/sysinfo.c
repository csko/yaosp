/* System information handling
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

#include <sysinfo.h>
#include <scheduler.h>
#include <process.h>
#include <thread.h>
#include <smp.h>
#include <module.h>
#include <mm/pages.h>

static process_listener_t* process_listener = NULL;

int set_process_listener( process_listener_t* listener ) {
    process_listener = listener;
    return 0;
}

int notify_process_listener( proc_listener_event_t event, process_t* process, thread_t* thread ) {
    if ( process_listener == NULL ) {
        return 0;
    }

    switch ( event ) {
        case PROCESS_CREATED :
            if ( process_listener->process_created != NULL ) {
                process_listener->process_created( process );
            }

            break;

        case PROCESS_DESTROYED :
            if ( process_listener->process_destroyed != NULL ) {
                process_listener->process_destroyed( process->id );
            }

            break;

        case PROCESS_RENAMED :
            if ( process_listener->process_renamed != NULL ) {
                process_listener->process_renamed( process );
            }

            break;

        case THREAD_CREATED :
            if ( process_listener->thread_created != NULL ) {
                process_listener->thread_created( thread );
            }

            break;

        case THREAD_DESTROYED :
            if ( process_listener->thread_destroyed != NULL ) {
                process_listener->thread_destroyed( thread->process->id, thread->id );
            }

            break;

        case THREAD_RENAMED :
            if ( process_listener->thread_renamed != NULL ) {
                process_listener->thread_renamed( thread );
            }

            break;
    }

    return 0;
}

int sys_get_system_info( system_info_t* system_info ) {
    system_info->total_page_count = get_total_page_count();
    system_info->free_page_count = get_free_page_count();

    system_info->total_processor_count = processor_count;
    system_info->active_processor_count = get_active_processor_count();

    system_info->loaded_module_count = get_loaded_module_count();

    spinlock_disable( &scheduler_lock );

    system_info->process_count = get_process_count();
    system_info->thread_count = get_thread_count();

    spinunlock_enable( &scheduler_lock );

    return 0;
}
