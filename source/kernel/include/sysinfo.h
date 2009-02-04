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

typedef struct system_info {
    /* Memory information */

    uint32_t free_page_count;
    uint32_t total_page_count;

    /* Processor information */

    uint32_t total_processor_count;
    uint32_t active_processor_count;
} system_info_t;

typedef struct kernel_info {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t release_version;
    char build_date[ 32 ];
    char build_time[ 32 ];
} kernel_info_t;

int sys_get_system_info( system_info_t* system_info );
int sys_get_kernel_info( kernel_info_t* kernel_info );

#endif // _SYSINFO_H_
