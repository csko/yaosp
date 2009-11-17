/* Thread implementation
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

#include <thread.h>
#include <errno.h>
#include <kernel.h>
#include <smp.h>
#include <macros.h>
#include <console.h>
#include <signal.h>
#include <debug.h>
#include <sched/scheduler.h>
#include <mm/kmalloc.h>
#include <mm/pages.h>
#include <lib/hashtable.h>
#include <lib/string.h>

#include <arch/pit.h> /* get_system_time() */
#include <arch/thread.h>
#include <arch/spinlock.h>
#include <arch/interrupt.h>

typedef struct thread_info_iter_data {
    uint32_t curr_index;
    uint32_t max_count;
    thread_info_t* info_table;
} thread_info_iter_data_t;

hashtable_t thread_table;

static int thread_id_counter = 0;

thread_t* allocate_thread( const char* name, process_t* process, int priority, uint32_t kernel_stack_pages ) {
    int i;
    int error;
    thread_t* thread;
    struct sigaction* handler;

    thread = ( thread_t* )kmalloc( sizeof( thread_t ) );

    if ( thread == NULL ) {
        goto error1;
    }

    memset( thread, 0, sizeof( thread_t ) );

    thread->name = strdup( name );

    if ( thread->name == NULL ) {
        goto error2;
    }

    thread->kernel_stack_pages = kernel_stack_pages;
    thread->kernel_stack = ( void* )alloc_pages( kernel_stack_pages, MEM_COMMON );

    if ( thread->kernel_stack == NULL ) {
        goto error3;
    }

    thread->kernel_stack_end = ( uint8_t* )thread->kernel_stack + ( kernel_stack_pages * PAGE_SIZE );

    error = arch_allocate_thread( thread );

    if ( error < 0 ) {
        goto error4;
    }

    thread->id = -1;
    thread->parent_id = -1;
    thread->state = THREAD_NEW;
    thread->priority = priority;
    thread->process = process;
    thread->user_stack_end = NULL;
    thread->user_stack_region = NULL;

    atomic_inc( &process->thread_count );

    reset_thread_quantum( thread );

    /* Set signal handlers to default */

    for ( i = 0, handler = &thread->signal_handlers[ 0 ]; i < _NSIG - 1; i++, handler++ ) {
        handler->sa_handler = SIG_DFL;
        handler->sa_flags = 0;
    }

    /* Ignoring SIGCHLD means automatic zombie cleaning */

    thread->signal_handlers[ SIGCHLD - 1 ].sa_handler = SIG_IGN;

    return thread;

error4:
    free_pages( thread->kernel_stack, kernel_stack_pages );

error3:
    kfree( thread->name );

error2:
    kfree( thread );

error1:
    return NULL;
}

void destroy_thread( thread_t* thread ) {
    /* Destroy the architecture dependent part of the thread */

    arch_destroy_thread( thread );

    /* Delete the userspace stack region */

    if ( thread->user_stack_region != NULL ) {
        memory_region_put( thread->user_stack_region );
        thread->user_stack_region = NULL;
    }

    /* Free the kernel stack */

    free_pages( thread->kernel_stack, thread->kernel_stack_pages );

    /* Destroy the process as well if this is the last thread */

    if ( atomic_dec_and_test( &thread->process->thread_count ) ) {
        /* Remove the process from the hashtable */

        scheduler_lock();
        remove_process( thread->process );
        scheduler_unlock();

        /* Destroy the process */

        destroy_process( thread->process );
    }

    /* Free other resources allocated by the thread */

    kfree( thread->name );
    kfree( thread );
}

int insert_thread( thread_t* thread ) {
    int error;

    ASSERT( scheduler_is_locked() );

    do {
        thread->id = thread_id_counter++;

        if ( thread_id_counter < 0 ) {
            thread_id_counter = 0;
        }
    } while ( hashtable_get( &thread_table, ( const void* )&thread->id ) != NULL );

    error = hashtable_add( &thread_table, ( hashitem_t* )thread );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int rename_thread( thread_t* thread, char* new_name ) {
    char* name;

    ASSERT( scheduler_is_locked() );

    name = strdup( new_name );

    if ( name == NULL ) {
        return -ENOMEM;
    }

    kfree( thread->name );
    thread->name = name;

    return 0;
}

static int thread_reparent_iterator( hashitem_t* item, void* data ) {
    thread_id parent;
    thread_t* thread;

    thread = ( thread_t* )item;
    parent = *( ( thread_id* )data );

    if ( thread->parent_id == parent ) {
        thread->parent_id = init_thread_id;
    }

    return 0;
}

void thread_exit( int exit_code ) {
    thread_t* thread;

    /* Disable interrupts to make sure the timer interrupt won't preempt us */

    disable_interrupts();

    /* Lock the scheduler. We set the thread state to zombie, so the scheduler
       won't try to run it again. */

    spinlock( &scheduler_lock );

    thread = current_thread();
    thread->state = THREAD_ZOMBIE;
    thread->exit_code = exit_code;

    if ( __likely( thread->parent_id != -1 ) ) {
        thread_t* parent;

        parent = get_thread_by_id( thread->parent_id );

        if ( __unlikely( parent == NULL ) ) {
            kprintf( ERROR, "thread_exit(): Thread parent not found!\n" );
        } else {
            do_send_signal( parent, SIGCHLD );
        }
    } else {
        kprintf(
            WARNING,
            "thread_exit(): Thread %s:%s with parent id -1 tried to exit!\n",
            thread->process->name,
            thread->name
        );
    }

    /* Reparent the children of the current thread */

    hashtable_iterate( &thread_table, thread_reparent_iterator, ( void* )&thread->id );

    spinunlock( &scheduler_lock );

    /* Make sure we don't try to execute this thread anymore */

    while ( 1 ) {
        sched_preempt();
    }
}

void kernel_thread_exit( void ) {
    thread_exit( 0 );
}

thread_id create_kernel_thread( const char* name, int priority, thread_entry_t* entry, void* arg, uint32_t stack_size ) {
    int error;
    thread_t* thread;
    thread_t* current;
    process_t* kernel_process;

    /* Get the kernel process */

    scheduler_lock();

    kernel_process = get_process_by_id( 0 );

    scheduler_unlock();

    if ( kernel_process == NULL ) {
        error = -EINVAL;
        goto error1;
    }

    /* Calculate kernel stack size */

    stack_size = PAGE_ALIGN( stack_size );

    if ( stack_size == 0 ) {
        stack_size = KERNEL_STACK_PAGES;
    } else {
        stack_size /= PAGE_SIZE;
    }

    /* Allocate a new thread */

    thread = allocate_thread( name, kernel_process, priority, stack_size );

    if ( thread == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    /* Initialize the architecture dependent part of the thread */

    error = arch_create_kernel_thread( thread, ( void* )entry, arg );

    if ( error < 0 ) {
        goto error2;
    }

    thread->in_system = 1;

    current = current_thread();

    if ( current != NULL ) {
        thread->parent_id = current->id;
    }

    /* Get an unique ID to the new thread and add to the others */

    scheduler_lock();

    error = insert_thread( thread );

    if ( error >= 0 ) {
        error = thread->id;
    }

    scheduler_unlock();

    if ( error < 0 ) {
        goto error2;
    }

    return error;

error2:
    destroy_thread( thread );

error1:
    return error;
}

thread_id sys_create_thread( const char* name, int priority, thread_entry_t* entry, void* arg, uint32_t user_stack_size ) {
    int error;
    thread_t* thread;
    thread_t* current;
    process_t* process;
    uint8_t* user_stack;

    /* Get the current process */

    current = current_thread();
    process = current->process;

    /* Calculate user stack size */

    if ( user_stack_size == 0 ) {
        user_stack_size = USER_STACK_SIZE;
    } else {
        user_stack_size = PAGE_ALIGN( user_stack_size );
    }

    /* Allocate a new thread */

    thread = allocate_thread( name, process, priority, KERNEL_STACK_PAGES );

    if ( thread == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    thread->user_stack_region = memory_region_create(
        "stack",
        user_stack_size,
        REGION_READ | REGION_WRITE | REGION_STACK
    );

    if ( thread->user_stack_region == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    memory_region_alloc_pages( thread->user_stack_region ); /* todo: check return value */

    user_stack = ( uint8_t* )thread->user_stack_region->address;
    thread->user_stack_end = ( void* )( user_stack + user_stack_size );

    /* Initialize the architecture dependent part of the thread */

    error = arch_create_user_thread( thread, ( void* )entry, arg );

    if ( error < 0 ) {
        goto error2;
    }

    thread->in_system = 0;
    thread->parent_id = current->id;

    /* Get an unique ID to the new thread and add to the others */

    scheduler_lock();

    error = insert_thread( thread );

    if ( error >= 0 ) {
        error = thread->id;
    }

    scheduler_unlock();

    if ( error < 0 ) {
        goto error2;
    }

    return error;

error2:
    destroy_thread( thread );

error1:
    return error;
}

int sys_exit_thread( int exit_code ) {
    thread_exit( exit_code );

    return 0;
}

static int do_sleep_thread( uint64_t microsecs, uint64_t* remaining ) {
    int error;
    thread_t* thread;
    waitnode_t node;
    uint64_t now;

    ASSERT( !is_interrupts_disabled() );

    node.wakeup_time = get_system_time() + microsecs;
    node.in_queue = false;

    scheduler_lock();

    thread = current_thread();

    thread->state = THREAD_SLEEPING;
    node.thread = thread->id;

    waitqueue_add_node( &sleep_queue, &node );

    scheduler_unlock();
    sched_preempt();
    scheduler_lock();

    waitqueue_remove_node( &sleep_queue, &node );

    scheduler_unlock();

    now = get_system_time();

    if ( now < node.wakeup_time ) {
        if ( remaining != NULL ) {
            *remaining = node.wakeup_time - now;
        }

        error = -ETIME;
    } else {
        error = 0;
    }

    return error;
}

int thread_sleep( uint64_t microsecs ) {
    return do_sleep_thread( microsecs, NULL );
}

static int thread_info_process_filter( hashitem_t* item, void* data ) {
    process_id id;
    thread_t* thread;

    id = *( ( process_id* )data );
    thread = ( thread_t* )item;

    if ( thread->process->id != id ) {
        return -EINVAL;
    }

    return 0;
}

static int get_thread_info_iterator( hashitem_t* item, void* _data ) {
    thread_t* thread;
    thread_info_t* info;
    thread_info_iter_data_t* data;

    thread = ( thread_t* )item;
    data = ( thread_info_iter_data_t* )_data;

    if ( data->curr_index >= data->max_count ) {
        return 0;
    }

    info = ( thread_info_t* )&data->info_table[ data->curr_index ];

    info->id = thread->id;
    strncpy( info->name, thread->name, MAX_THREAD_NAME_LENGTH );
    info->name[ MAX_THREAD_NAME_LENGTH - 1 ] = 0;
    info->state = thread->state;
    info->cpu_time = thread->cpu_time;

    data->curr_index++;

    return 0;
}

uint32_t sys_get_thread_count_for_process( process_id id ) {
    uint32_t count;

    scheduler_lock();

    count = hashtable_get_filtered_item_count( &thread_table, thread_info_process_filter, ( void* )&id );

    scheduler_unlock();

    return count;
}

uint32_t sys_get_thread_info_for_process( process_id id, thread_info_t* info_table, uint32_t max_count ) {
    thread_info_iter_data_t data;

    data.curr_index = 0;
    data.max_count = max_count;
    data.info_table = info_table;

    scheduler_lock();

    hashtable_filtered_iterate(
        &thread_table,
        get_thread_info_iterator,
        ( void* )&data,
        thread_info_process_filter,
        ( void* )&id
    );

    scheduler_unlock();

    return data.curr_index;
}

int sys_sleep_thread( uint64_t* microsecs, uint64_t* remaining ) {
    return do_sleep_thread( *microsecs, remaining );
}

int do_wake_up_thread( thread_t* thread ) {
    int error;

    ASSERT( scheduler_is_locked() );

    if ( ( thread->state == THREAD_NEW ) ||
         ( thread->state == THREAD_WAITING ) ||
         ( thread->state == THREAD_SLEEPING ) ) {
        add_thread_to_ready( thread );
        error = 0;
    } else {
        error = -EINVAL;
    }

    return error;
}

int thread_wake_up( thread_id id ) {
    int error;
    thread_t* thread;

    scheduler_lock();

    thread = get_thread_by_id( id );

    if ( thread != NULL ) {
        error = do_wake_up_thread( thread );
    } else {
        error = -EINVAL;
    }

    scheduler_unlock();

    return error;
}

int sys_wake_up_thread( thread_id id ) {
    int error;
    thread_t* thread;

    scheduler_lock();

    thread = get_thread_by_id( id );

    if ( thread != NULL ) {
        error = do_wake_up_thread( thread );
    } else {
        error = -EINVAL;
    }

    scheduler_unlock();

    return error;
}

thread_id sys_gettid( void ) {
    return current_thread()->id;
}

uint32_t get_thread_count( void ) {
    ASSERT( scheduler_is_locked() );

    return hashtable_get_item_count( &thread_table );
}

thread_t* get_thread_by_id( thread_id id ) {
    ASSERT( scheduler_is_locked() );

    return ( thread_t* )hashtable_get( &thread_table, ( const void* )&id );
}

static void* thread_key( hashitem_t* item ) {
    thread_t* thread;

    thread = ( thread_t* )item;

    return ( void* )&thread->id;
}

#ifdef ENABLE_DEBUGGER
static const char* thread_states[] = {
    "unknown",
    "new",
    "ready",
    "running",
    "waiting",
    "sleeping",
    "zombie"
};

static int dbg_list_thread_iterator( hashitem_t* item, void* data ) {
    thread_t* thread;

    thread = ( thread_t* )item;

    dbg_printf(
        "%4d %-25s %-25s %s\n",
        thread->id,
        thread->process->name,
        thread->name,
        thread_states[ thread->state ]
    );

    return 0;
}

int dbg_list_threads( const char* params ) {
    dbg_set_scroll_mode( true );

    dbg_printf( "  Id Process                   Thread                   State\n" );
    dbg_printf( "-----------------------------------------------------------------\n" );

    hashtable_iterate( &thread_table, dbg_list_thread_iterator, NULL );

    dbg_set_scroll_mode( false );

    return 0;
}

int dbg_show_thread_info( const char* params ) {
    thread_id id;
    thread_t* thread;

    if ( params == NULL ) {
        dbg_printf( "show-thread-info thread_id\n" );

        return 0;
    }

    if ( !str_to_num( params, &id ) ) {
        dbg_printf( "Thread ID must be a number!\n" );

        return 0;
    }

    thread = ( thread_t* )hashtable_get( &thread_table, ( const void* )&id );

    if ( thread == NULL ) {
        dbg_printf( "Thread %d not found!\n", id );

        return 0;
    }

    dbg_printf( "Informations about thread %d:\n", id );
    dbg_printf( "  process: %s\n", thread->process->name );
    dbg_printf( "  name: %s\n", thread->name );
    dbg_printf( "  state: %s\n", thread_states[ thread->state ] );
    dbg_printf( "  priority: %d\n", thread->priority );

    if ( thread->blocking_semaphore != -1 ) {
        dbg_printf( "  blocking semaphore: %d\n", thread->blocking_semaphore );
    }

    arch_dbg_show_thread_info( thread );

    return 0;
}

int dbg_trace_thread( const char* params ) {
    thread_id id;
    thread_t* thread;

    if ( params == NULL ) {
        dbg_printf( "trace-thread thread_id\n" );

        return 0;
    }

    if ( !str_to_num( params, &id ) ) {
        dbg_printf( "Thread ID must be a number!\n" );

        return 0;
    }

    thread = ( thread_t* )hashtable_get( &thread_table, ( const void* )&id );

    if ( thread == NULL ) {
        dbg_printf( "Thread %d not found!\n", id );

        return 0;
    }

    dbg_set_scroll_mode( true );
    arch_dbg_trace_thread( thread );
    dbg_set_scroll_mode( false );

    return 0;
}
#endif /* ENABLE_DEBUGGER */

__init int init_threads( void ) {
    int error;

    error = init_hashtable(
        &thread_table,
        256,
        thread_key,
        hash_int,
        compare_int
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
