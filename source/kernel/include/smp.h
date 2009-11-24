/* Symmetric multi-processing
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
 * Copyright (c) 2009 Kornel Csernai
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

#ifndef _SMP_H_
#define _SMP_H_

#include <config.h>
#include <thread.h>
#include <process.h>

#include <arch/atomic.h>

typedef struct cpu {
    char name[ MAX_PROCESSOR_NAME_LENGTH ];

    bool present;
    volatile bool running;
    uint64_t core_speed;

    thread_t* current_thread;
    thread_t* idle_thread;
    uint64_t idle_time;

    void* arch_data;
    uint32_t features;
} cpu_t;

typedef struct processor_info {
    char name[ MAX_PROCESSOR_NAME_LENGTH ];
    uint64_t core_speed;
    int present;
    int running;
    uint32_t features;
} processor_info_t;

extern int processor_count;
extern atomic_t active_processor_count;
extern cpu_t processor_table[ MAX_CPU_COUNT ];

cpu_t* get_processor( void );

process_t* current_process( void );
thread_t* current_thread( void );
thread_t* idle_thread( void );

int get_active_processor_count( void );

uint32_t sys_get_processor_count( void );
uint32_t sys_get_processor_info( processor_info_t* info_table, uint32_t max_count );

int init_smp( void );
int init_smp_late( void );

#endif /* _SMP_H_ */
