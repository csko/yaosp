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

#include <errno.h>

#include <yaosp/time.h>
#include <yaosp/sysinfo.h>
#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

int get_kernel_info( kernel_info_t* kernel_info ) {
    return syscall1( SYS_get_kernel_info, ( int )kernel_info );
}

uint32_t get_module_count( void ) {
    return syscall0( SYS_get_module_count );
}

int get_module_info( module_info_t* info, uint32_t max_count ) {
    return syscall2( SYS_get_module_info, ( int )info, max_count );
}

uint32_t get_process_count( void ) {
    return syscall0( SYS_get_process_count );
}

uint32_t get_process_info( process_info_t* info, uint32_t max_count ) {
    return syscall2( SYS_get_process_info, ( int )info, max_count );
}

uint32_t get_thread_count_for_process( process_id id ) {
    return syscall1( SYS_get_thread_count_for_process, id );
}

uint32_t get_thread_info_for_process( process_id id, thread_info_t* info_table, uint32_t max_count ) {
    return syscall3( SYS_get_thread_info_for_process, id, ( int )info_table, max_count );
}

uint32_t get_processor_count( void ) {
    return syscall0( SYS_get_processor_count );
}

uint32_t get_processor_info( processor_info_t* info_table, uint32_t max_count ) {
    return syscall2( SYS_get_processor_info, ( int )info_table, max_count );
}

int get_memory_info( memory_info_t* info ) {
    return syscall1( SYS_get_memory_info, ( int )info );
}

time_t get_system_time( void ) {
    int error;
    time_t tmp;

    error = syscall1( SYS_get_system_time, ( int )&tmp );

    if ( error < 0 ) {
        errno = -error;
        return 0;
    }

    return tmp;
}

time_t get_boot_time( void ) {
    int error;
    time_t tmp;

    error = syscall1( SYS_get_boot_time, ( int )&tmp );

    if ( error < 0 ) {
        errno = -error;
        return 0;
    }

    return tmp;
}
