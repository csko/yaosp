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
#include <version.h>
#include <mm/pages.h>
#include <lib/string.h>

int sys_get_system_info( system_info_t* system_info ) {
    system_info->total_page_count = get_total_page_count();
    system_info->free_page_count = get_free_page_count();

    system_info->total_processor_count = processor_count;
    system_info->active_processor_count = get_active_processor_count();

    spinlock_disable( &scheduler_lock );

    spinunlock_enable( &scheduler_lock );

    return 0;
}

int sys_get_kernel_info( kernel_info_t* kernel_info ) {
    kernel_info->major_version = KERNEL_MAJOR_VERSION;
    kernel_info->minor_version = KERNEL_MINOR_VERSION;
    kernel_info->release_version = KERNEL_RELEASE_VERSION;

    strncpy( kernel_info->build_date, build_date, 32 );
    strncpy( kernel_info->build_time, build_time, 32 );

    kernel_info->build_date[ 31 ] = 0;
    kernel_info->build_time[ 31 ] = 0;

    return 0;
}
