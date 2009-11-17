/* Miscellaneous kernel functions
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#ifndef _KERNEL_H_
#define _KERNEL_H_

#include <types.h>
#include <thread.h>

#define __init \
    __attribute__(( section( ".kernel_init" ) ))

#define panic( format, arg... ) \
    handle_panic( __FILE__, __LINE__, format, ##arg )

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

extern int __ro_end;
extern int __data_start;
extern int __kernel_end;

extern thread_id init_thread_id;

int parse_kernel_parameters( const char* params );

int get_kernel_param_as_string( const char* key, const char** value );
int get_kernel_param_as_bool( const char* key, bool* value );

int sys_get_kernel_info( kernel_info_t* kernel_info );
int sys_get_kernel_statistics( statistics_info_t* statistics_info );

int sys_dbprintf( const char* format, char** parameters );

void handle_panic( const char* file, int line, const char* format, ... );

int init_thread( void* arg );

void reboot( void );
void shutdown( void );

int sys_reboot( void );
int sys_shutdown( void );

int arch_late_init( void );
void kernel_main( void );
int create_init_thread( void );

#endif /* _KERNEL_H_ */
