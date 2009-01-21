/* ps shell command
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

#define ALLOC_NUM 32

typedef struct thread_info {
    int tid;
    char* name;
} thread_info_t;

typedef struct process_info {
    int pid;
    char* name;
    thread_info_t* threads;
    int numthreads;
} process_info_t;

static int p_asc(const void* _p1, const void* _p2){
    process_info_t* p1 = (process_info_t*) _p1;
    process_info_t* p2 = (process_info_t*) _p2;

    return p1->pid - p2->pid;
}

/*
static int p_desc(const void* _p1, const void* _p2){
    process_info_t* p1 = (process_info_t*) _p1;
    process_info_t* p2 = (process_info_t*) _p2;

    return p2->pid - p1->pid;
}
*/

static int t_asc(const void* _t1, const void* _t2){
    thread_info_t* t1 = (thread_info_t*) _t1;
    thread_info_t* t2 = (thread_info_t*) _t2;

    return t1->tid - t2->tid;
}

/*
static int t_desc(const void* _t1, const void* _t2){
    thread_info_t* t1 = (thread_info_t*) _t1;
    thread_info_t* t2 = (thread_info_t*) _t2;

    return t2->tid - t1->tid;
}
*/

process_info_t* procs = NULL;
size_t numprocs = 0;

static void print_thread(thread_info_t* thread){
    printf( "%4s %4d  `- %s\n", "", thread->tid, thread->name );
}

static void print_proc(process_info_t* proc){
    int j;

    printf( "%4d %4s %s\n", proc->pid, "-", proc->name );

    if(proc->numthreads > 0){
        /* Sort the threads */
        qsort(proc->threads, proc->numthreads, sizeof(proc->threads[0]), t_asc);
    }

    /* Print threads */
    for(j = 0; j < proc->numthreads; j++){
        print_thread(&(proc->threads[j]));
    }

}

/* Free all the allocated memory */
static void destroy_procs(){
    int i,j;

    for(i = 0; i < numprocs; i++){
        free(procs[i].name);

        for(j = 0; j < procs[i].numthreads; j++){
            free(procs[i].threads[j].name);
        }

        free(procs[i].threads);
    }

    free(procs);

}

static int read_proc_entry_node( char* dir, char* node, char* buffer, size_t size ) {
    int fd;
    int data;
    char path[ 64 ];

    snprintf( path, sizeof( path ), "/process/%s/%s", dir, node );

    fd = open( path, O_RDONLY );

    if ( fd < 0 ) {
        return fd;
    }

    data = read( fd, buffer, size - 1 );

    close( fd );

    if ( data < 0 ) {
        return data;
    }

    buffer[ data ] = 0;

    return 0;
}

static int do_ts( char* thread, char* tid, process_info_t* proc ) {
    int error;
    char name[ 128 ];
    thread_info_t* threads = proc->threads;
    thread_info_t* tmp_threads;
    int numthreads = proc->numthreads;

    error = read_proc_entry_node( thread, "name", name, sizeof( name ) );

    if ( error < 0 ) {
        return error;
    }

    if(numthreads % ALLOC_NUM == 0){
        tmp_threads = (thread_info_t*) realloc(threads, sizeof(thread_info_t) * (numthreads + ALLOC_NUM));

        if(tmp_threads == NULL){
            return -ENOMEM;
        }

        threads = tmp_threads;

    }

    threads[numthreads].name = strdup(name);
    threads[numthreads].tid = atoi(tid);
    proc->numthreads++;
    proc->threads = threads;

    return 0;
}

static int do_ps( char* process ) {
    int error;
    char name[ 128 ];
    char path[ 128 ];
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stat;
    process_info_t* tmp_procs;

    error = read_proc_entry_node( process, "name", name, sizeof( name ) );

    if ( error < 0 ) {
        return 0;
    }

    if(numprocs % ALLOC_NUM == 0){
        tmp_procs = (process_info_t*) realloc(procs, sizeof(process_info_t) * (numprocs + ALLOC_NUM));

        if(tmp_procs == NULL){
            return -ENOMEM;
        }

        procs = tmp_procs;

    }

    procs[numprocs].pid = atoi(process);
    procs[numprocs].name = strdup(name);
    procs[numprocs].numthreads = 0;
    procs[numprocs].threads = NULL;
    numprocs++;

    snprintf( path, sizeof( path ), "/process/%s", process );

    dir = opendir( path );

    if ( dir == NULL ) {
        return 0;
    }

    while ( ( entry = readdir( dir ) ) != NULL ) {
        snprintf( path, sizeof( path ), "/process/%s/%s", process, entry->d_name );

        if ( stat( path, &entry_stat ) != 0 ) {
            continue;
        }

        if ( !S_ISDIR( entry_stat.st_mode ) ) {
            continue;
        }

        snprintf( path, sizeof( path ), "%s/%s", process, entry->d_name );
        if( do_ts( path, entry->d_name, &procs[numprocs - 1] ) == -ENOMEM ){
            return -ENOMEM;
        }
    }

    closedir( dir );
    return 0;
}

int main( int argc, char** argv ) {
    DIR* dir;
    struct dirent* entry;
    int i;

    dir = opendir( "/process" );

    if ( dir == NULL ) {
        fprintf (stderr, "%s: Failed to open /process!\n", argv[0] );
        return EXIT_FAILURE;
    }

    printf( " PID  TID NAME\n" );

    while ( ( entry = readdir( dir ) ) != NULL ) {
        if(do_ps( entry->d_name ) == -ENOMEM ){
            fprintf (stderr, "%s: Failed to allocate memory for process list!\n", argv[0] );

            closedir(dir);

            return EXIT_FAILURE;
        }
    }

    if(numprocs > 0){
        /* Sort the processes */
        qsort(procs, numprocs, sizeof(procs[0]), p_asc);

        /* Print the processes */
        for(i = 0; i < numprocs; i++){
            print_proc(&procs[i]);
        }
    }

    closedir( dir );
    destroy_procs();

    return EXIT_SUCCESS;
}
