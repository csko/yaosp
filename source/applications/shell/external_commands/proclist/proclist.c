/* proclist shell command
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
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <yaosp/sysinfo.h>

static char* argv0 = NULL;

static int p_asc( const void* _p1, const void* _p2  ) {
    process_info_t* p1 = ( process_info_t* )_p1;
    process_info_t* p2 = ( process_info_t* )_p2;

    return ( p1->id - p2->id );
}

/*
static int p_desc( const void* _p1, const void* _p2 ) {
    process_info_t* p1 = ( process_info_t* )_p1;
    process_info_t* p2 = ( process_info_t* )_p2;

    return ( p2->id - p1->id );
}
*/

static int t_asc( const void* _t1, const void* _t2 ) {
    thread_info_t* t1 = ( thread_info_t* )_t1;
    thread_info_t* t2 = ( thread_info_t* )_t2;

    return ( t1->id - t2->id );
}

/*
static int t_desc( const void* _t1, const void* _t2 ) {
    thread_info_t* t1 = ( thread_info_t* )_t1;
    thread_info_t* t2 = ( thread_info_t* )_t2;

    return ( t2->id - t1->id );
}
*/

static void format_size( char* buffer, size_t length, uint64_t size ) {
    if ( size < 1024 ) {
        snprintf( buffer, length, "%4u b ", ( unsigned int )size );
    } else if ( size < ( 1024 * 1024 ) ) {
        snprintf( buffer, length, "%4u Kb", ( unsigned int )( size / 1024 ) );
    } else if ( size < ( 1024 * 1024 * 1024 ) ) {
        snprintf( buffer, length, "%4u Mb", ( unsigned int )( size / ( 1024 * 1024 ) ) );
    } else {
        snprintf( buffer, length, "%4u Gb", ( unsigned int )( size / ( 1024 * 1024 * 1024 ) ) );
    }
}

static void print_thread( thread_info_t* thread ) {
    printf( "%4s %4d                  `- %s\n", "", thread->id, thread->name );
}

static void print_process( process_info_t* process ) {
    char vmem_str[ 32 ];
    char pmem_str[ 32 ];
    uint32_t thread_count;
    thread_info_t* thread_table;

    format_size( vmem_str, sizeof( vmem_str ), process->vmem_size );
    format_size( pmem_str, sizeof( pmem_str ), process->pmem_size );

    printf( "%4d %4s %s %s %s\n", process->id, "-", vmem_str, pmem_str, process->name );

    thread_count = get_thread_count_for_process( process->id );

    if ( thread_count > 0 ) {
        uint32_t i;

        thread_table = ( thread_info_t* )malloc( sizeof( thread_info_t ) * thread_count );

        if ( thread_table == NULL ) {
            fprintf( stderr, "%s: No memory for thread table!\n", argv0 );
            return;
        }

        thread_count = get_thread_info_for_process( process->id, thread_table, thread_count );

        qsort( thread_table, thread_count, sizeof( thread_info_t ), t_asc );

        for ( i = 0; i < thread_count; i++ ) {
            print_thread( &thread_table[ i ] );
        }

        free( thread_table );
    }
}

int main( int argc, char** argv ) {
    uint32_t process_count;
    process_info_t* process_table;

    argv0 = argv[ 0 ];

    process_count = get_process_count();

    if ( process_count > 0 ) {
        uint32_t i;

        /* Get the process list */

        process_table = ( process_info_t* )malloc( sizeof( process_info_t ) * process_count );

        if ( process_table == NULL ) {
            fprintf( stderr, "%s: No memory for process table!\n", argv0 );
            return EXIT_SUCCESS;
        }

        process_count = get_process_info( process_table, process_count );

        /* Sort the process list */

        qsort( process_table, process_count, sizeof( process_info_t ), p_asc );

        /* Print the header */

        printf( " PID  TID VIRTMEM PHYSMEM NAME\n" );

        /* Print the process list */

        for ( i = 0; i < process_count; i++ ) {
            print_process( &process_table[ i ] );
        }

        free( process_table );
    }

    return EXIT_SUCCESS;
}
