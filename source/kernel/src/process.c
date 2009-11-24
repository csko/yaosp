/* Process implementation
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

#include <process.h>
#include <errno.h>
#include <smp.h>
#include <macros.h>
#include <kernel.h>
#include <console.h>
#include <sched/scheduler.h>
#include <mm/kmalloc.h>
#include <mm/context.h>
#include <vfs/vfs.h>
#include <lib/hashtable.h>
#include <lib/string.h>

typedef struct process_info_iter_data {
    uint32_t curr_index;
    uint32_t max_count;
    process_info_t* info_table;
} process_info_iter_data_t;

static int process_id_counter = 0;
static hashtable_t process_table;

process_t* allocate_process( char* name ) {
    process_t* process;

    process = ( process_t* )kmalloc( sizeof( process_t ) );

    if ( process == NULL ) {
        goto error1;
    }

    memset( process, 0, sizeof( process_t ) );

    process->name = strdup( name );

    if ( process->name == NULL ) {
        goto error2;
    }

    process->mutex = mutex_create( "process mutex", MUTEX_NONE );

    if ( process->mutex < 0 ) {
        goto error3;
    }

    process->id = -1;
    process->heap_region = NULL;

    atomic_set( &process->thread_count, 0 );

    return process;

error3:
    kfree( process->name );

error2:
    kfree( process );

error1:
    return NULL;
}

void destroy_process( process_t* process ) {
    /* NOTE: We don't delete the heap region here because the call to
             the memory_context_delete_regions() will do it for us! */

    /* Destroy the application loader related structures */

    if ( process->loader != NULL ) {
        process->loader->destroy( process->loader_data );
    }

    /* Destroy the memory context */

    if ( process->memory_context != NULL ) {
        memory_context_delete_regions( process->memory_context );
        memory_context_destroy( process->memory_context );
    }

    /* Destroy the I/O context */

    if ( process->io_context != NULL ) {
        destroy_io_context( process->io_context );
    }

    /* Destroy the locking context */

    if ( process->lock_context != NULL ) {
        lock_context_destroy( process->lock_context );
    }

    /* Delete other resources allocated by the process */

    mutex_destroy( process->mutex );
    kfree( process->name );
    kfree( process );
}

int insert_process( process_t* process ) {
    int error;

    ASSERT( scheduler_is_locked() );

    do {
        process->id = process_id_counter++;

        if ( process_id_counter < 0 ) {
            process_id_counter = 0;
        }
    } while ( hashtable_get( &process_table, ( const void* )&process->id ) != NULL );

    error = hashtable_add( &process_table, ( hashitem_t* )process );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

void remove_process( process_t* process ) {
    ASSERT( scheduler_is_locked() );

    hashtable_remove( &process_table, ( const void* )&process->id );
}

int rename_process( process_t* process, char* new_name ) {
    char* name;

    ASSERT( scheduler_is_locked() );

    name = strdup( new_name );

    if ( name == NULL ) {
        return -ENOMEM;
    }

    kfree( process->name );
    process->name = name;

    return 0;
}

uint32_t get_process_count( void ) {
    ASSERT( scheduler_is_locked() );

    return hashtable_get_item_count( &process_table );
}

process_t* get_process_by_id( process_id id ) {
    ASSERT( scheduler_is_locked() );

    return ( process_t* )hashtable_get( &process_table, ( const void* )&id );
}

uint32_t sys_get_process_count( void ) {
    uint32_t result;

    scheduler_lock();

    result = hashtable_get_item_count( &process_table );

    scheduler_unlock();

    return result;
}

static int get_process_info_iterator( hashitem_t* item, void* _data ) {
    process_t* process;
    process_info_t* info;
    process_info_iter_data_t* data;

    process = ( process_t* )item;
    data = ( process_info_iter_data_t* )_data;

    if ( data->curr_index >= data->max_count ) {
        return 0;
    }

    info = ( process_info_t* )&data->info_table[ data->curr_index ];

    info->id = process->id;
    strncpy( info->name, process->name, MAX_PROCESS_NAME_LENGTH );
    info->name[ MAX_PROCESS_NAME_LENGTH - 1 ] = 0;
    info->pmem_size = process->pmem_size;
    info->vmem_size = process->vmem_size;

    data->curr_index++;

    return 0;
}

uint32_t sys_get_process_info( process_info_t* info_table, uint32_t max_count ) {
    process_info_iter_data_t data;

    data.curr_index = 0;
    data.max_count = max_count;
    data.info_table = info_table;

    scheduler_lock();

    hashtable_iterate( &process_table, get_process_info_iterator, ( void* )&data );

    scheduler_unlock();

    return data.curr_index;
}

process_id sys_getpid( void ) {
    return current_process()->id;
}

int sys_exit( int exit_code ) {
    process_t* process;

    process = current_process();

    /* TODO: Send KILL signal to all thread of this process */

    thread_exit( exit_code );

    return 0;
}

typedef struct wait4_chld_chk_data {
    thread_id parent;
    thread_t** child;
    bool zombie_only;
} wait4_chld_chk_data_t;

static int wait4_child_check_iterator( hashitem_t* item, void* _data ) {
    thread_t* thread;
    wait4_chld_chk_data_t* data;

    thread = ( thread_t* )item;
    data = ( wait4_chld_chk_data_t* )_data;

    if ( ( data->zombie_only ) &&
         ( thread->state != THREAD_ZOMBIE ) ) {
        return 0;
    }

    if ( thread->parent_id == data->parent ) {
        *data->child = thread;

        return -1;
    }

    return 0;
}

static thread_t* find_and_remove_zombie_child( thread_id parent_id, process_id pid ) {
    thread_t* tmp;

    ASSERT( scheduler_is_locked() );

    if ( pid > 0 ) {
        tmp = get_thread_by_id( pid );

        /* Is this thread zombie? */

        if ( ( tmp != NULL ) &&
             ( tmp->state != THREAD_ZOMBIE ) ) {
            tmp = NULL;
        }
    } else if ( pid == -1 ) {
        wait4_chld_chk_data_t data;

        tmp = NULL;

        data.parent = parent_id;
        data.child = &tmp;
        data.zombie_only = true;

        hashtable_iterate( &thread_table, wait4_child_check_iterator, ( void* )&data );
    } else {
        kprintf( WARNING, "find_and_remove_zombie_child(): Group support not yet implemented!\n" );

        tmp = NULL;
    }

    if ( tmp != NULL ) {
        hashtable_remove( &thread_table, ( const void* )&tmp->id );
    }

    return tmp;
}

process_id sys_wait4( process_id pid, int* status, int options, struct rusage* rusage ) {
    int error;
    thread_t* tmp;
    thread_t* current;

    current = current_thread();

    /* Make sure we have something to wait for */

    scheduler_lock();

    if ( pid > 0 ) {
        tmp = get_thread_by_id( pid );
    } else if ( pid == -1 ) {
        wait4_chld_chk_data_t data;

        tmp = NULL;

        data.parent = current->id;
        data.child = &tmp;
        data.zombie_only = false;

        hashtable_iterate( &thread_table, wait4_child_check_iterator, ( void* )&data );
    } else {
        kprintf( WARNING, "sys_wait4(): wait4 for groups not yet implemented!\n" );

        tmp = NULL;
    }

    scheduler_unlock();

    if ( tmp == NULL ) {
        return -ECHILD;
    }

    while ( 1 ) {
        scheduler_lock();

        tmp = find_and_remove_zombie_child( current->id, pid );

        if ( tmp != NULL ) {
            scheduler_unlock();

            error = tmp->id;

            if ( status != NULL ) {
                /* TODO: this makes bash unhappy on non-existing commands ... *status = tmp->exit_code; */
                *status = 0;
            }

            if ( rusage != NULL ) {
                struct timeval* tv;

                tv = &rusage->ru_utime;

                tv->tv_sec = tmp->user_time / 1000000;
                tv->tv_usec = tmp->user_time % 1000000;

                DEBUG_LOG( "Thread: %s:%s\n", tmp->process->name, tmp->name );
                DEBUG_LOG( "System time: %llu.%llu\n", tv->tv_sec, tv->tv_usec );

                tv = &rusage->ru_stime;

                tv->tv_sec = tmp->sys_time / 1000000;
                tv->tv_usec = tmp->user_time % 1000000;

                DEBUG_LOG( "User time: %llu.%llu\n", tv->tv_sec, tv->tv_usec );
            }

            destroy_thread( tmp );

            return error;
        }

        if ( options & WNOHANG ) {
            scheduler_unlock();

            return 0;
        }

        if ( is_signal_pending( current ) ) {
            scheduler_unlock();

            return -EINTR;
        }

        current->state = THREAD_WAITING;

        scheduler_unlock();

        sched_preempt();
    }

    /* This is never reached :) */

    return -EINVAL;
}

int sys_getrusage( int who, struct rusage* usage ) {
    thread_t* thread;
    struct timeval* tv;

    thread = current_thread();

    switch ( who ) {
        case RUSAGE_SELF :
            scheduler_lock();

            tv = &usage->ru_utime;
            tv->tv_sec = thread->user_time / 1000000;
            tv->tv_usec = thread->user_time % 1000000;

            tv = &usage->ru_stime;
            tv->tv_sec = thread->sys_time / 1000000;
            tv->tv_usec = thread->sys_time % 1000000;

            scheduler_unlock();

            /* TODO: add time usage from other threads as well */

            break;

        case RUSAGE_CHILDREN :
            kprintf(
                WARNING,
                "sys_getrusage(): RUSAGE_CHILDREN not yet implemented!\n"
            );

            return -ENOSYS;

        case RUSAGE_THREAD :
            scheduler_lock();

            tv = &usage->ru_utime;
            tv->tv_sec = thread->user_time / 1000000;
            tv->tv_usec = thread->user_time % 1000000;

            tv = &usage->ru_stime;
            tv->tv_sec = thread->sys_time / 1000000;
            tv->tv_usec = thread->sys_time % 1000000;

            scheduler_unlock();

            break;

        default :
            kprintf(
                WARNING,
                "sys_getrusage(): Invalid request: %d\n",
                who
            );

            return -EINVAL;
    }

    return 0;
}

static void* process_key( hashitem_t* item ) {
    process_t* process;

    process = ( process_t* )item;

    return ( void* )&process->id;
}

__init int init_processes( void ) {
    int error;
    process_t* process;

    /* Initialize the process hashtable */

    error = init_hashtable(
        &process_table,
        128,
        process_key,
        hash_int,
        compare_int
    );

    if ( error < 0 ) {
        return error;
    }

    /* Create kernel process */

    process = allocate_process( "kernel" );

    if ( process == NULL ) {
        return -ENOMEM;
    }

    process->memory_context = &kernel_memory_context;
    process->lock_context = &kernel_lock_context;
    process->io_context = &kernel_io_context;

    /* TODO: This vmem setting is highly architecture dependent! */
    process->vmem_size = MIN( get_total_page_count() * PAGE_SIZE, 512 * 1024 * 1024 );

    kernel_memory_context.process = process;

    scheduler_lock();
    error = insert_process( process );
    scheduler_unlock();

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
