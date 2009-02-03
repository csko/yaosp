/* System information application
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <yaosp/sysinfo.h>

char* argv0 = NULL;

static void do_get_kernel_info( void ) {
    int error;
    kernel_info_t kernel_info;

    error = get_kernel_info( &kernel_info );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to get kernel information!\n", argv0 );
        return;
    }

    printf(
        "Kernel version: %d.%d.%d\n",
        kernel_info.major_version,
        kernel_info.minor_version,
        kernel_info.release_version
    );
    printf(
        "Kernel was built on %s %s\n",
        kernel_info.build_date,
        kernel_info.build_time
    );
}

static void do_get_module_info( void ) {
    int error;
    uint32_t i;
    uint32_t module_count;
    module_info_t* info_table;

    module_count = get_module_count();

    printf( "Loaded modules: %u\n", module_count );

    if ( module_count > 0 ) {
        info_table = ( module_info_t* )malloc( sizeof( module_info_t ) * module_count );

        if ( info_table == NULL ) {
            return;
        }

        error = get_module_info( info_table, module_count );

        if ( error < 0 ) {
            goto out;
        }

        for ( i = 0; i < module_count; i++ ) {
            printf( "%s\n", info_table[ i ].name );
        }

out:
        free( info_table );
    }
}

int main( int argc, char** argv ) {
    int error;
    system_info_t sysinfo;

    argv0 = argv[ 0 ];

    error = get_system_info( &sysinfo );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to get system information!\n", argv0 );
        return EXIT_FAILURE;
    }

    printf( "Total memory: %d Kb\n", ( sysinfo.total_page_count * getpagesize() ) / 1024 );
    printf( "Free memory: %d Kb\n", ( sysinfo.free_page_count * getpagesize() ) / 1024 );
    printf( "Process count: %d\n", sysinfo.process_count );
    printf( "Thread count: %d\n", sysinfo.thread_count );
    printf( "Total CPUs: %d\n", sysinfo.total_processor_count );
    printf( "Active CPUs: %d\n", sysinfo.active_processor_count );

    do_get_kernel_info();
    do_get_module_info();

    return EXIT_SUCCESS;
}
