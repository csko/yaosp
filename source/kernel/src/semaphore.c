/* Semaphore implementation
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

#include <types.h>
#include <errno.h>
#include <semaphore.h>
#include <smp.h>
#include <errno.h>
#include <scheduler.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

#include <arch/pit.h> /* get_system_time() */
#include <arch/interrupt.h>

#define GLOBAL ( 1 << 24 )
#define ID_MASK 0x00FFFFFF

semaphore_context_t global_semaphore_context;
semaphore_context_t kernel_semaphore_context;

static semaphore_t* allocate_semaphore( const char* name ) {
    semaphore_t* semaphore;

    semaphore = ( semaphore_t* )kmalloc( sizeof( semaphore_t ) );

    if ( semaphore == NULL ) {
        return NULL;
    }

    memset( semaphore, 0, sizeof( semaphore_t ) );

    semaphore->name = strdup( name );

    if ( semaphore->name == NULL ) {
        kfree( semaphore );
        return NULL;
    }

    return semaphore;
}

static void free_semaphore( semaphore_t* semaphore ) {
    kfree( semaphore->name );
    kfree( semaphore );
}

semaphore_id do_create_semaphore( bool kernel, const char* name, semaphore_type_t type, uint32_t flags, int count ) {
    int error;
    semaphore_id id;
    semaphore_t* semaphore;
    semaphore_context_t* context;

    /* Allocate a new semaphore */

    semaphore = allocate_semaphore( name );

    if ( semaphore == NULL ) {
        return -ENOMEM;
    }
    /* Initialize other parts of the semaphore */

    error = init_waitqueue( &semaphore->waiters );

    if ( error < 0 ) {
        free_semaphore( semaphore );
        return error;
    }

    semaphore->type = type;
    semaphore->flags = flags;

    switch ( type ) {
        case SEMAPHORE_BINARY :
            semaphore->count = 1;
            break;

        case SEMAPHORE_COUNTING :
            semaphore->count = count;
            break;
    }

    /* Decide which semaphore context to use */

    if ( flags & SEMAPHORE_GLOBAL ) {
        context = &global_semaphore_context;
    } else if ( kernel ) {
        context = &kernel_semaphore_context;
    } else {
        context = current_process()->semaphore_context;
    }

    /* Insert the new semaphore to the context */

    spinlock_disable( &context->lock );

    do {
        semaphore->id = context->semaphore_id_counter;

        context->semaphore_id_counter = ( context->semaphore_id_counter + 1 ) & 0x00FFFFFF;
    } while ( hashtable_get( &context->semaphore_table, ( const void* )semaphore->id ) != NULL );

    hashtable_add( &context->semaphore_table, ( hashitem_t* )semaphore );

    id = semaphore->id;

    spinunlock_enable( &context->lock );

    if ( flags & SEMAPHORE_GLOBAL ) {
        id |= GLOBAL;
    }

    return id;
}

semaphore_id create_semaphore( const char* name, semaphore_type_t type, uint32_t flags, int count ) {
    return do_create_semaphore( true, name, type, flags, count );
}

semaphore_id sys_create_semaphore( const char* name, semaphore_type_t type, uint32_t flags, int count ) {
    return do_create_semaphore( false, name, type, flags, count );
}

static int do_delete_semaphore( bool kernel, semaphore_id id ) {
    semaphore_t* semaphore;
    semaphore_context_t* context;

    if ( id & GLOBAL ) {
        context = &global_semaphore_context;
    } else if ( kernel ) {
        context = &kernel_semaphore_context;
    } else {
        context = current_process()->semaphore_context;
    }

    /* First we remove the semaphore from the context */

    spinlock_disable( &context->lock );

    semaphore = ( semaphore_t* )hashtable_get( &context->semaphore_table, ( const void* )( id & ID_MASK ) );

    if ( semaphore == NULL ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    hashtable_remove( &context->semaphore_table, ( const void* )( id & ID_MASK ) );

    spinunlock_enable( &context->lock );

    /* Wake up threads waiting for this semaphore */

    spinlock_disable( &scheduler_lock );

    waitqueue_wake_up_all( &semaphore->waiters );

    spinunlock_enable( &scheduler_lock );

    /* Free the resources allocated by the semaphore */

    free_semaphore( semaphore );

    return 0;
}

int delete_semaphore( semaphore_id id ) {
    return do_delete_semaphore( true, id );
}

int sys_delete_semaphore( semaphore_id id ) {
    return do_delete_semaphore( false, id );
}

static int do_lock_semaphore( bool kernel, semaphore_id id, int count, uint64_t timeout ) {
    uint64_t wakeup_time;
    semaphore_t* semaphore;
    semaphore_context_t* context;

    ASSERT( !is_interrupts_disabled() );

    /* Check if semaphore ID is valid */

    if ( id < 0 ) {
        return -EINVAL;
    }

    /* Decide which context to use */

    if ( id & GLOBAL ) {
        context = &global_semaphore_context;
    } else if ( kernel ) {
        context = &kernel_semaphore_context;
    } else {
        context = current_process()->semaphore_context;
    }

    wakeup_time = get_system_time() + timeout;

    spinlock_disable( &context->lock );

    semaphore = ( semaphore_t* )hashtable_get( &context->semaphore_table, ( const void* )( id & ID_MASK ) );

    if ( semaphore == NULL ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    switch ( ( int )semaphore->type ) {
        case SEMAPHORE_BINARY :
            count = 1;
            break;
    }

try_again:
    if ( semaphore->count < count ) {
        thread_t* thread;
        waitnode_t waitnode;
        waitnode_t sleepnode;

        if ( ( timeout != INFINITE_TIMEOUT ) && ( wakeup_time <= get_system_time() ) ) {
            spinunlock_enable( &context->lock );

            return -ETIME;
        }

        spinlock( &scheduler_lock );

        thread = current_thread();

        waitnode.thread = thread->id;

        waitqueue_add_node_tail( &semaphore->waiters, &waitnode );

        if ( timeout != INFINITE_TIMEOUT ) {
            sleepnode.thread = thread->id;
            sleepnode.wakeup_time = wakeup_time;

            waitqueue_add_node( &sleep_queue, &sleepnode );
        }

        thread->state = THREAD_WAITING;

        spinunlock( &scheduler_lock );
        spinunlock_enable( &context->lock );

        sched_preempt();

        spinlock_disable( &context->lock );

        if ( timeout != INFINITE_TIMEOUT ) {
            spinlock( &scheduler_lock );

            waitqueue_remove_node( &sleep_queue, &sleepnode );

            spinunlock( &scheduler_lock );
        }

        semaphore = ( semaphore_t* )hashtable_get( &context->semaphore_table, ( const void* )( id & ID_MASK ) );

        if ( semaphore == NULL ) {
            spinunlock_enable( &context->lock );

            return -EINVAL;
        }

        waitqueue_remove_node( &semaphore->waiters, &waitnode );

        goto try_again;
    }

    semaphore->count -= count;

    spinunlock_enable( &context->lock );

    return 0;
}

static int do_unlock_semaphore( bool kernel, semaphore_id id, int count ) {
    semaphore_t* semaphore;
    semaphore_context_t* context;

    /* Check if semaphore is valid */

    if ( id < 0 ) {
        return -EINVAL;
    }

    /* Decide which context to use */

    if ( id & GLOBAL ) {
        context = &global_semaphore_context;
    } else if ( kernel ) {
        context = &kernel_semaphore_context;
    } else {
        context = current_process()->semaphore_context;
    }

    spinlock_disable( &context->lock );

    semaphore = ( semaphore_t* )hashtable_get( &context->semaphore_table, ( const void* )( id & ID_MASK ) );

    if ( semaphore == NULL ) {
        spinunlock_enable( &context->lock );

        return -EINVAL;
    }

    switch ( ( int )semaphore->type ) {
        case SEMAPHORE_BINARY :
            count = 1;
            break;
    }

    semaphore->count += count;

    spinlock( &scheduler_lock );

    waitqueue_wake_up_head( &semaphore->waiters, count );

    spinunlock( &scheduler_lock );

    spinunlock_enable( &context->lock );

    return 0;
}

int lock_semaphore( semaphore_id id, int count, uint64_t timeout ) {
    return do_lock_semaphore( true, id, count, timeout );
}

int sys_lock_semaphore( semaphore_id id, int count, uint64_t* timeout ) {
    return do_lock_semaphore( false, id, count, *timeout );
}

int unlock_semaphore( semaphore_id id, int count ) {
    return do_unlock_semaphore( true, id, count );
}

int sys_unlock_semaphore( semaphore_id id, int count ) {
    return do_unlock_semaphore( false, id, count );
}

static bool do_is_semaphore_locked( bool kernel, semaphore_id id ) {
    bool result;
    semaphore_t* semaphore;
    semaphore_context_t* context;

    /* Check if semaphore is valid */

    if ( id < 0 ) {
        return false;
    }

    /* Decide which context to use */

    if ( id & GLOBAL ) {
        context = &global_semaphore_context;
    } else if ( kernel ) {
        context = &kernel_semaphore_context;
    } else {
        context = current_process()->semaphore_context;
    }

    spinlock_disable( &context->lock );

    semaphore = ( semaphore_t* )hashtable_get( &context->semaphore_table, ( const void* )( id & ID_MASK ) );

    if ( semaphore == NULL ) {
        spinunlock_enable( &context->lock );

        return false;
    }

    result = ( semaphore->count == 0 );

    spinunlock_enable( &context->lock );

    return result;
}

bool is_semaphore_locked( semaphore_id id ) {
    return do_is_semaphore_locked( true, id );
}

static void* semaphore_key( hashitem_t* item ) {
    semaphore_t* semaphore;

    semaphore = ( semaphore_t* )item;

    return ( void* )semaphore->id;
}

static uint32_t semaphore_hash( const void* key ) {
    return ( uint32_t )key;
}

static bool semaphore_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

int init_semaphore_context( semaphore_context_t* context ) {
    int error;

    /* Initialize the spinlock */

    error = init_spinlock( &context->lock, "Semaphore context" );

    if ( error < 0 ) {
        return error;
    }

    /* Initialize the ID counter and the hashtable */

    context->semaphore_id_counter = 0;

    error = init_hashtable(
        &context->semaphore_table,
        32,
        semaphore_key,
        semaphore_hash,
        semaphore_compare
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

void destroy_semaphore_context( semaphore_context_t* context ) {
    /* TODO: make the table empty */
    destroy_hashtable( &context->semaphore_table );
    kfree( context );
}

semaphore_context_t* semaphore_context_clone( semaphore_context_t* old_context ) {
    int error;
    semaphore_context_t* new_context;

    new_context = ( semaphore_context_t* )kmalloc( sizeof( semaphore_context_t ) );

    if ( new_context == NULL ) {
        return NULL;
    }

    error = init_semaphore_context( new_context );

    if ( error < 0 ) {
        kfree( new_context );
        return NULL;
    }

    /* TODO: clone semaphores */

    return new_context;
}

int init_semaphores( void ) {
    int error;

    /* Initialize global semaphore context */

    error = init_semaphore_context( &global_semaphore_context );

    if ( error < 0 ) {
        return error;
    }

    /* Initialize the kernel semaphore context */

    error = init_semaphore_context( &kernel_semaphore_context );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
