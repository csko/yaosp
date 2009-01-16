/* System information application
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <yaosp/sysinfo.h>

int main( int argc, char** argv ) {
    int error;
    system_info_t sysinfo;

    error = get_system_info( &sysinfo );

    if ( error < 0 ) {
        printf( "Failed to get system information!\n" );
        return EXIT_FAILURE;
    }

    printf( "Total memory: %d Kb\n", ( sysinfo.total_page_count * getpagesize() ) / 1024 );
    printf( "Free memory: %d Kb\n", ( sysinfo.free_page_count * getpagesize() ) / 1024 );
    printf( "Process count: %d\n", sysinfo.process_count );
    printf( "Thread count: %d\n", sysinfo.thread_count );
    printf( "Active CPUs: %d\n", sysinfo.active_processor_count );

    return EXIT_SUCCESS;
}
