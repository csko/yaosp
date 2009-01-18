/* Symmetric multi-processing
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

typedef struct cpu {
    char name[ 255 ];

    bool present;
    volatile bool running;
    uint64_t core_speed;

    thread_t* current_thread;
    thread_t* idle_thread;

    void* arch_data;
} cpu_t;

extern cpu_t processor_table[ MAX_CPU_COUNT ];

process_t* current_process( void );
thread_t* current_thread( void );
thread_t* idle_thread( void );

int get_active_processor_count( void );

int init_smp( void );
int init_smp_late( void );

#endif // _SMP_H_
