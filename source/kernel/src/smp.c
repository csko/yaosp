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

#include <smp.h>
#include <types.h>
#include <thread.h>
#include <lib/string.h>

#include <arch/cpu.h>

cpu_t processor_table[ MAX_CPU_COUNT ];

thread_t* current_thread( void ) {
    return get_processor()->current_thread;
}

thread_t* idle_thread( void ) {
    return get_processor()->idle_thread;
}

int init_smp( void ) {
    memset( processor_table, 0, sizeof( cpu_t ) * MAX_CPU_COUNT );

    /* Make the boot CPU available */

    processor_table[ 0 ].present = true;
    processor_table[ 0 ].running = true;

    return 0;
}
