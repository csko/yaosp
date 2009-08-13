/* Locking context implementation
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

#include <macros.h>
#include <errno.h>
#include <console.h>
#include <mm/kmalloc.h>
#include <lock/context.h>
#include <lock/mutex.h>
#include <lock/semaphore.h>
#include <lock/condition.h>
#include <lock/rwlock.h>
#include <lib/string.h>

lock_context_t kernel_lock_context;

static size_t lock_size_table[ LOCK_COUNT ] = {
    sizeof( mutex_t ),
    sizeof( semaphore_t ),
    sizeof( condition_t ),
    sizeof( rwlock_t )
};

lock_header_t* lock_context_get( lock_context_t* context, lock_id id ) {
    ASSERT( spinlock_is_locked( &context->lock ) );

    return ( lock_header_t* )hashtable_get( &context->lock_table, ( const void* )&id );
}

int lock_context_insert( lock_context_t* context, lock_header_t* lock ) {
    int error;

    spinlock_disable( &context->lock );

    do {
        lock->id = context->next_lock_id++;

        if ( context->next_lock_id < 0 ) {
            context->next_lock_id = 0;
        }
    } while ( hashtable_get( &context->lock_table, ( const void* )&lock->id ) != NULL );

    error = hashtable_add( &context->lock_table, ( hashitem_t* )lock );

    spinunlock_enable( &context->lock );

    return error;
}

int lock_context_remove( lock_context_t* context, lock_id id, lock_type_t type, lock_header_t** _lock ) {
    int error;
    lock_header_t* lock;

    spinlock_disable( &context->lock );

    lock = ( lock_header_t* )hashtable_get( &context->lock_table, ( const void* )&id );

    if ( ( lock == NULL ) ||
         ( lock->type != type ) ) {
        error = -EINVAL;
        goto out;
    }

    hashtable_remove( &context->lock_table, ( const void* )&id );

    *_lock = lock;
    error = 0;

 out:
    spinunlock_enable( &context->lock );

    return error;
}

static int lock_context_clone_iterator( hashitem_t* hash, void* data ) {
    int error;
    size_t name_length;
    lock_header_t* header;
    lock_context_t* context;
    lock_header_t* new_header;

    header = ( lock_header_t* )hash;
    context = ( lock_context_t* )data;

    name_length = strlen( header->name );

    new_header = ( lock_header_t* )kmalloc( lock_size_table[ header->type ] + name_length + 1 );

    if ( new_header == NULL ) {
        return -ENOMEM;
    }

    new_header->name = ( char* )( new_header + 1 );

    new_header->type = header->type;
    new_header->id = header->id;
    memcpy( new_header->name, header->name, name_length + 1 );

    switch ( ( int )header->type ) {
        case MUTEX :
            error = mutex_clone( ( mutex_t* )header, ( mutex_t* )new_header );
            break;

        case SEMAPHORE :
            /* NOTE: semaphores are used only inside the kernel at the moment, so cloning not (yet) implemented! */
            return 0;

        case CONDITION :
            error = condition_clone( ( condition_t* )header, ( condition_t* )new_header );
            break;

        case RWLOCK :
            kprintf( WARNING, "lock_context_clone_iterator(): RW lock cloning not yet implemented!\n" );
            error = -ENOSYS;
            break;

        default :
            error = -EINVAL;
    }

    if ( error < 0 ) {
        return error;
    }

    hashtable_add( &context->lock_table, ( hashitem_t* )new_header );

    return 0;
}

lock_context_t* lock_context_clone( lock_context_t* old_context ) {
    int error;
    lock_context_t* new_context;

    new_context = ( lock_context_t* )kmalloc( sizeof( lock_context_t ) );

    if ( new_context == NULL ) {
        goto error1;
    }

    error = lock_context_init( new_context );

    if ( error < 0 ) {
        goto error2;
    }

    spinlock_disable( &old_context->lock );
    error = hashtable_iterate( &old_context->lock_table, lock_context_clone_iterator, ( void* )new_context );
    spinunlock_enable( &old_context->lock );

    if ( error < 0 ) {
        goto error3;
    }

    return new_context;

 error3:
    /* TODO: destroy the lock context */

 error2:
    kfree( new_context );

 error1:
    return NULL;
}

static int lock_context_update_iterator( hashitem_t* hash, void* data ) {
    int error;
    thread_id new_thread;
    lock_header_t* header;

    new_thread = *( ( thread_id* )data );
    header = ( lock_header_t* )hash;

    switch ( ( int )header->type ) {
        case MUTEX :
            error = mutex_update( ( mutex_t* )header, new_thread );
            break;

        case SEMAPHORE :
            error = 0; /* See the note at lock_context_clone_iterator() */
            break;

        case CONDITION :
            error = condition_update( ( condition_t* )header, new_thread );
            break;

        case RWLOCK :
            kprintf( WARNING, "lock_context_update_iterator(): RW lock update not yet implemented!\n" );
            error = -ENOSYS;
            break;

        default :
            error = -EINVAL;
            break;
    }

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int lock_context_update( lock_context_t* context, thread_id new_thread ) {
    int error;

    spinlock_disable( &context->lock );
    error = hashtable_iterate( &context->lock_table, lock_context_update_iterator, ( void* )&new_thread );
    spinunlock_enable( &context->lock );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int lock_context_make_empty_iterator( hashitem_t* item, void* data ) {
    lock_header_t* header;
    hashtable_t* lock_table;

    header = ( lock_header_t* )item;
    lock_table = ( hashtable_t* )data;

    hashtable_remove( lock_table, ( const void* )&header->id );
    /* TODO: free the lock! */

    return 0;
}

int lock_context_make_empty( lock_context_t* context ) {
     /* TODO: is this correct? */

    hashtable_iterate( &context->lock_table, lock_context_make_empty_iterator, ( void* )&context->lock_table );

    return 0;
}

static void* lock_key( hashitem_t* item ) {
    lock_header_t* lock;

    lock = ( lock_header_t* )item;

    return ( void* )&lock->id;
}

int lock_context_init( lock_context_t* context ) {
    int error;

    /* Initialize the spinlock */

    error = init_spinlock( &context->lock, "Semaphore context" );

    if ( error < 0 ) {
        return error;
    }

    /* Initialize the ID counter and the hashtable */

    context->next_lock_id = 0;

    error = init_hashtable(
        &context->lock_table,
        64,
        lock_key,
        hash_int,
        compare_int
    );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int lock_context_destroy( lock_context_t* context ) {
    return 0;
}

int init_locking( void ) {
    return lock_context_init( &kernel_lock_context );
}
