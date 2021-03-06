/* System information definitions
 *
 * Copyright (c) 2009 Zoltan Kovacs, Kornel Csernai
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

#ifndef _YAOSP_SYSINFO_H_
#define _YAOSP_SYSINFO_H_

#include <sys/types.h>

#define MAX_MODULE_NAME_LENGTH    64
#define MAX_PROCESS_NAME_LENGTH   64
#define MAX_THREAD_NAME_LENGTH    64
#define MAX_PROCESSOR_NAME_LENGTH 64

enum {
    THREAD_UNKNOWN = 0,
    THREAD_NEW,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_WAITING,
    THREAD_SLEEPING,
    THREAD_ZOMBIE
};

enum {
    CPU_FEATURE_MMX = ( 1 << 0 ),
    CPU_FEATURE_SSE = ( 1 << 1 ),
    CPU_FEATURE_APIC = ( 1 << 2 ),
    CPU_FEATURE_MTRR = ( 1 << 3 ),
    CPU_FEATURE_SYSCALL = ( 1 << 4 ),
    CPU_FEATURE_TSC = ( 1 << 5 ),
    CPU_FEATURE_SSE2 = ( 1 << 6 ),
    CPU_FEATURE_HTT = ( 1 << 7 ),
    CPU_FEATURE_SSE3 = ( 1 << 8 ),
    CPU_FEATURE_PAE = ( 1 << 9 ),
    CPU_FEATURE_IA64 = ( 1 << 10 ),
    CPU_FEATURE_EST = ( 1 << 11 )
};

typedef int process_id;
typedef int thread_id;

typedef struct kernel_info {
    uint32_t major_version;
    uint32_t minor_version;
    uint32_t release_version;
    char build_date[ 32 ];
    char build_time[ 32 ];
} kernel_info_t;

typedef struct statistics_info {
    uint32_t semaphore_count;
} statistics_info_t;

typedef struct module_info {
    uint32_t dependency_count;
    char name[ MAX_MODULE_NAME_LENGTH ];
} module_info_t;

typedef struct process_info {
    process_id id;
    char name[ MAX_PROCESS_NAME_LENGTH ];
    uint64_t pmem_size;
    uint64_t vmem_size;
} process_info_t;

typedef struct thread_info {
    thread_id id;
    char name[ MAX_THREAD_NAME_LENGTH ];
    int state;
    uint64_t cpu_time;
} thread_info_t;

typedef struct processor_info {
    char name[ MAX_PROCESSOR_NAME_LENGTH ];
    uint64_t core_speed;
    int present;
    int running;
    uint32_t features;
} processor_info_t;

typedef struct memory_info {
    uint32_t free_page_count;
    uint32_t total_page_count;
    uint32_t kmalloc_used_pages;
    uint32_t kmalloc_alloc_size;
} memory_info_t;

int get_kernel_info( kernel_info_t* kernel_info );
int get_kernel_statistics( statistics_info_t* statistics_info );

uint32_t get_module_count( void );
int get_module_info( module_info_t* info, uint32_t max_count );

uint32_t get_process_count( void );
uint32_t get_process_info( process_info_t* info, uint32_t max_count );

uint32_t get_thread_count_for_process( process_id id );
uint32_t get_thread_info_for_process( process_id id, thread_info_t* info_table, uint32_t max_count );

uint32_t get_processor_count( void );
uint32_t get_processor_info( processor_info_t* info_table, uint32_t max_count );

int get_memory_info( memory_info_t* info );

#endif /* _YAOSP_SYSINFO_H_ */
