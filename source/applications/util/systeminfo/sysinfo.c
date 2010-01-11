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

typedef struct i386_feature {
    int feature;
    const char* name;
} i386_feature_t;

i386_feature_t i386_features[] = {
    { CPU_FEATURE_MMX, "mmx" },
    { CPU_FEATURE_SSE, "sse" },
    { CPU_FEATURE_APIC, "apic" },
    { CPU_FEATURE_MTRR, "mtrr" },
    { CPU_FEATURE_SYSCALL, "syscall" },
    { CPU_FEATURE_TSC, "tsc" },
    { CPU_FEATURE_SSE2, "sse2" },
    { CPU_FEATURE_HTT, "htt" },
    { CPU_FEATURE_SSE3, "sse3" },
    { CPU_FEATURE_PAE, "pae" },
    { CPU_FEATURE_IA64, "ia64" },
    { CPU_FEATURE_EST, "est" },
    { 0, "" }
};

static void print_usage( void ) {
    printf( "%s INFO_TYPE ...\n", argv0 );
    printf( "\n" );
    printf( "The INFO_TYPE has to be one of these values:\n" );
    printf( "    kernel       Kernel information\n" );
    printf( "    module       Kernel module information\n" );
    printf( "    processor    Processor information\n" );
    printf( "    memory       Memory information\n" );
    printf( "    statistics   Kernel statistics\n" );
    printf( "    all          All of the above\n" );
}

static int do_get_kernel_info( void ) {
    int error;
    kernel_info_t kernel_info;

    error = get_kernel_info( &kernel_info );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to get kernel information!\n", argv0 );
        return EXIT_FAILURE;
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

    return EXIT_SUCCESS;
}

static int do_get_kernel_statistics( void ) {
    int error;
    statistics_info_t statistics;

    error = get_kernel_statistics( &statistics );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to get kernel statistics!\n", argv0 );
        return EXIT_FAILURE;
    }

    printf(
        "Semaphore count: %u\n",
        statistics.semaphore_count
    );

    return EXIT_SUCCESS;
}

static int do_get_module_info( void ) {
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
            return EXIT_FAILURE;
        }

        error = get_module_info( info_table, module_count );

        if ( error < 0 ) {
            fprintf( stderr, "%s: Failed to get module information!\n", argv0 );
            free( info_table );
            return EXIT_FAILURE;
        }

        for ( i = 0; i < module_count; i++ ) {
            printf( "%s\n", info_table[ i ].name );
        }

        free( info_table );
    }

    return EXIT_SUCCESS;
}

static int do_get_processor_info( void ) {
    uint32_t i;
    uint32_t j;
    uint32_t processor_count;
    processor_info_t* info;
    processor_info_t* info_table;

    processor_count = get_processor_count();

    if ( processor_count == 0 ) {
        printf( "No processors.\n" );
        return EXIT_SUCCESS;
    }

    info_table = ( processor_info_t* )malloc( sizeof( processor_info_t ) * processor_count );

    if ( info_table == NULL ) {
        fprintf( stderr, "%s: No memory for processor info table!\n", argv0 );
        return EXIT_FAILURE;
    }

    processor_count = get_processor_info( info_table, processor_count );

    for ( i = 0; i < processor_count; i++ ) {
        info = &info_table[ i ];

        if ( ( !info->present ) || ( !info->running ) ) {
            continue;
        }

        if ( i > 0 ) {
            printf( "\n" );
        }

        printf( "Processor:    %u\n", i );
        printf( "Model name:   %s\n", info->name );
        printf( "Speed:        %u MHz\n", ( uint32_t )( info->core_speed / 1000000 ) );
        printf( "Features:" );

        for ( j = 0; i386_features[ j ].feature != 0; j++ ) {
            if ( ( info->features & i386_features[ j ].feature ) != 0 ) {
                printf( " %s", i386_features[ j ].name );
            }
        }

        printf( "\n" );
    }

    free( info_table );

    return EXIT_SUCCESS;
}

static int do_get_memory_info( void ) {
    int error;
    int pagesize;
    memory_info_t memory_info;

    error = get_memory_info( &memory_info );

    if ( error < 0 ) {
        fprintf( stderr, "%s: Failed to get memory information!\n", argv0 );
        return EXIT_FAILURE;
    }

    pagesize = getpagesize();

    printf( "System memory\n" );
    printf( "  total: %u Kb\n", ( memory_info.total_page_count * pagesize / 1024 ) );
    printf( "  free:  %u Kb\n", ( memory_info.free_page_count * pagesize / 1024 ) );
    printf( "\n" );
    printf( "Kmalloc memory\n" );
    printf( "  used:      %u Kb\n", ( memory_info.kmalloc_used_pages * pagesize / 1024 ) );
    printf( "  allocated: %u Kb\n", ( memory_info.kmalloc_alloc_size / 1024 ) );

    return EXIT_SUCCESS;
}

int main( int argc, char** argv ) {
    char* info_type;
    int i;
    int ret = EXIT_SUCCESS;

    argv0 = argv[ 0 ];

    if ( argc <= 1 ) {
        print_usage();
        return EXIT_FAILURE;
    }

    for ( i = 1; i < argc; i++ ) {
        info_type = argv[ i ];

        if ( strcmp( info_type, "kernel" ) == 0 ) {
            if ( do_get_kernel_info() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
        } else if ( strcmp( info_type, "module" ) == 0 ) {
            if ( do_get_module_info() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
        } else if ( strcmp( info_type, "processor" ) == 0 ) {
            if ( do_get_processor_info() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
        } else if ( strcmp( info_type, "memory" ) == 0 ) {
            if ( do_get_memory_info() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
        } else if ( strcmp( info_type, "statistics" ) == 0 ) {
            if ( do_get_kernel_statistics() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
        } else if ( strcmp( info_type, "all" ) == 0 ) {
            if ( do_get_kernel_info() != EXIT_SUCCESS ){
                ret = EXIT_FAILURE;
            }
            if ( do_get_module_info() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
            if ( do_get_processor_info() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
            if ( do_get_memory_info() != EXIT_SUCCESS ) {
                ret = EXIT_FAILURE;
            }
        } else {
            /* Maybe use preprocessing of arguments? */
            print_usage();
            return EXIT_FAILURE;
        }
    }

    return ret;
}
