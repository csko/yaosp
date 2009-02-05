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

static void print_usage( void ) {
    printf( "%s info_type\n", argv0 );
    printf( "\n" );
    printf( "The info_type has to be one of these values:\n" );
    printf( "    kernel - Kernel information\n" );
    printf( "    module - Kernel module information\n" );
    printf( "    processor - Processor information\n" );
    printf( "    memory - Memory information\n" );
}

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
            fprintf( stderr, "%s: No memory for module info table!\n", argv0 );
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

static void do_get_processor_info( void ) {
    uint32_t i;
    uint32_t processor_count;
    processor_info_t* info;
    processor_info_t* info_table;

    processor_count = get_processor_count();

    if ( processor_count == 0 ) {
        return;
    }

    info_table = ( processor_info_t* )malloc( sizeof( processor_info_t ) * processor_count );

    if ( info_table == NULL ) {
        fprintf( stderr, "%s: No memory for processor info table!\n", argv0 );
        return;
    }

    processor_count = get_processor_info( info_table, processor_count );

    for ( i = 0; i < processor_count; i++ ) {
        info = &info_table[ i ];

        if ( ( !info->present ) || ( !info->running ) ) {
            continue;
        }

        printf( "Processor:  %u\n", i );
        printf( "Model name: %s\n", info->name );
        printf( "Speed:      %u MHz\n\n", ( uint32_t )( info->core_speed / 1000000 ) );
    }

    free( info_table );
}

static void do_get_memory_info( void ) {
    int error;
    memory_info_t memory_info;

    error = get_memory_info( &memory_info );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to get memory information!\n", argv0 );
        return;
    }

    printf( "Total memory: %u Kb\n", ( memory_info.total_page_count * getpagesize() / 1024 ) );
    printf( "Free memory: %u Kb\n", ( memory_info.free_page_count * getpagesize() / 1024 ) );
}

int main( int argc, char** argv ) {
    char* info_type;

    argv0 = argv[ 0 ];

    if ( argc != 2 ) {
        print_usage();
        return EXIT_FAILURE;
    }

    info_type = argv[ 1 ];

    if ( strcmp( info_type, "kernel" ) == 0 ) {
        do_get_kernel_info();
    } else if ( strcmp( info_type, "module" ) == 0 ) {
        do_get_module_info();
    } else if ( strcmp( info_type, "processor" ) == 0 ) {
        do_get_processor_info();
    } else if ( strcmp( info_type, "memory" ) == 0 ) {
        do_get_memory_info();
    } else {
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
