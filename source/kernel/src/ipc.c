/* Inter Process Communication
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#include <ipc.h>
#include <config.h>
#include <macros.h>
#include <errno.h>
#include <console.h>
#include <smp.h>
#include <lock/mutex.h>
#include <lock/semaphore.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

static lock_id ipc_port_mutex;
static int ipc_port_id_counter;
static hashtable_t ipc_port_table;

static lock_id named_ipc_port_mutex;
static hashtable_t named_ipc_port_table;

static int ipc_port_insert( ipc_port_t* port ) {
    int error;

    /* Save the process ID to the IPC port */

    port->owner_id = current_process()->id;

    /* Find an unused ID for the port and insert to the table */

    do {
        port->id = ipc_port_id_counter++;

        if ( ipc_port_id_counter < 0 ) {
            ipc_port_id_counter = 0;
        }
    } while ( hashtable_get( &ipc_port_table, ( const void* )&port->id ) != NULL );

    error = hashtable_add( &ipc_port_table, ( hashitem_t* )port );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

static int ipc_port_remove( ipc_port_id id, ipc_port_t** _port ) {
    ipc_port_t* port;

    port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&id );

    if ( port == NULL ) {
        return -EINVAL;
    }

    /* Check if this process is the owner of the IPC port or not */

    if ( port->owner_id != current_process()->id ) {
        return -EPERM;
    }

    hashtable_remove( &ipc_port_table, ( const void* )&id );

    *_port = port;

    return 0;
}

ipc_port_id sys_create_ipc_port( void ) {
    int error;
    ipc_port_t* port;

    port = ( ipc_port_t* )kmalloc( sizeof( ipc_port_t ) );

    if ( port == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    port->queue_semaphore = semaphore_create( "IPC queue semaphore", 0 );

    if ( port->queue_semaphore < 0 ) {
        error = port->queue_semaphore;
        goto error2;
    }

    port->proc_next = NULL;
    port->message_queue = NULL;
    port->message_queue_tail = NULL;

    mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    /* Insert the port to the global table */

    error = ipc_port_insert( port );

    if ( error >= 0 ) {
        process_t* process = current_process();

        /* Link the port to the process as well */

        port->proc_next = process->ipc_port_list;
        process->ipc_port_list = port;
    }

    mutex_unlock( ipc_port_mutex );

    if ( error < 0 ) {
        goto error3;
    }

    return port->id;

 error3:
    semaphore_destroy( port->queue_semaphore );

 error2:
    kfree( port );

 error1:
    return error;
}

static int do_destroy_ipc_port( ipc_port_t* port ) {
    while ( port->message_queue != NULL ) {
        ipc_message_t* msg = port->message_queue;
        port->message_queue = msg->next;

        kfree( msg );
    }

    semaphore_destroy( port->queue_semaphore );
    kfree( port );

    return 0;
}

int ipc_destroy_process_ports( process_t* process ) {
    ipc_port_t* current;

    mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    current = process->ipc_port_list;

    while ( current != NULL ) {
        ipc_port_t* to_delete = current;
        current = current->proc_next;

        hashtable_remove( &ipc_port_table, ( const void* )&to_delete->id );
        do_destroy_ipc_port( to_delete );
    }

    mutex_unlock( ipc_port_mutex );

    return 0;
}

int sys_destroy_ipc_port( ipc_port_id port_id ) {
    int error;
    ipc_port_t* port = NULL;

    mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    /* Remove the port from the global table. */

    error = ipc_port_remove( port_id, &port );

    /* If everything was OK, then we remove the port from
       the owner process' list as well. */

    if ( error == 0 ) {
        process_t* process = current_process();
        ipc_port_t* prev = NULL;
        ipc_port_t* current = process->ipc_port_list;

        while ( current != NULL ) {
            if ( current == port ) {
                if ( prev == NULL ) {
                    process->ipc_port_list = current->proc_next;
                } else {
                    prev->proc_next = current->proc_next;
                }

                break;
            }

            prev = current;
            current = current->proc_next;
        }
    }

    mutex_unlock( ipc_port_mutex );

    if ( error == 0 ) {
        do_destroy_ipc_port( port );
    }

    return 0;
}

int sys_send_ipc_message( ipc_port_id port_id, uint32_t code, void* data, size_t size ) {
    int error;
    ipc_port_t* port;
    ipc_message_t* message;

    mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

    if ( port == NULL ) {
        error = -EINVAL;
        goto error1;
    }

    message = ( ipc_message_t* )kmalloc( sizeof( ipc_message_t ) + size );

    if ( message == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    message->code = code;
    message->size = size;

    if ( size > 0 ) {
        memcpy( ( void* )( message + 1 ), data, size );
    }

    message->next = NULL;

    if ( port->message_queue == NULL ) {
        ASSERT( port->message_queue_tail == NULL );

        port->message_queue = message;
        port->message_queue_tail = message;
    } else {
        ASSERT( port->message_queue_tail != NULL );

        port->message_queue_tail->next = message;
        port->message_queue_tail = message;
    }

    semaphore_unlock( port->queue_semaphore, 1 );
    mutex_unlock( ipc_port_mutex );

    return 0;

error1:
    mutex_unlock( ipc_port_mutex );

    return error;
}

int sys_recv_ipc_message( ipc_port_id port_id, uint32_t* code, void* buffer, size_t size, uint64_t* _timeout ) {
    int error;
    uint64_t timeout;
    ipc_port_t* port;
    ipc_message_t* message;

    timeout = *_timeout;

    mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

    if ( port == NULL ) {
        error = -EINVAL;
        goto error2;
    }

    if (port->message_queue == NULL) {
        mutex_unlock( ipc_port_mutex );

        error = semaphore_timedlock( port->queue_semaphore, 1, LOCK_IGNORE_SIGNAL, timeout );

        if ( error < 0 ) {
            goto error1;
        }

        mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

        port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

        if ( port == NULL ) {
            error = -EINVAL;
            goto error2;
        }
    } else {
        error = semaphore_timedlock( port->queue_semaphore, 1, LOCK_IGNORE_SIGNAL, 0 );

        if (error != 0) {
            kprintf(
                ERROR,
                "sys_recv_ipc_message(): Failed to lock semaphore (%d) while the queue was not empty: %d!\n",
                port->queue_semaphore,
                error
            );
        }
    }

    if (port->message_queue == NULL) {
        error = -ENOENT;
        goto error2;
    }

    message = port->message_queue;

    if ( message->size > size ) {
        error = -E2BIG;
        goto error2;
    }

    port->message_queue = message->next;

    if ( port->message_queue == NULL ) {
        port->message_queue_tail = NULL;
    }

    mutex_unlock( ipc_port_mutex );

    if ( code != NULL ) {
        *code = message->code;
    }

    if ( message->size > 0 ) {
        ASSERT( message->size <= size );
        memcpy( buffer, ( void* )( message + 1 ), message->size );
    }

    error = message->size;

    kfree( message );

    return error;

 error2:
    mutex_unlock( ipc_port_mutex );

 error1:
    return error;
}

int sys_peek_ipc_message( ipc_port_id port_id, uint32_t* code, size_t* size, uint64_t* _timeout ) {
    int error;
    ipc_port_t* port;
    uint64_t timeout;
    ipc_message_t* message;

    timeout = *_timeout;

    mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

    if ( port == NULL ) {
        error = -EINVAL;
        goto error2;
    }

    if ( port->message_queue == NULL ) {
        mutex_unlock( ipc_port_mutex );

        error = semaphore_timedlock( port->queue_semaphore, 1, LOCK_IGNORE_SIGNAL, timeout );

        if ( error < 0 ) {
            goto error1;
        }

        mutex_lock( ipc_port_mutex, LOCK_IGNORE_SIGNAL );

        port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

        if ( port == NULL ) {
            error = -EINVAL;
            goto error2;
        }
    } else {
        error = semaphore_timedlock( port->queue_semaphore, 1, LOCK_IGNORE_SIGNAL, 0 );

        if ( error != 0 ) {
            kprintf( ERROR, "sys_peek_ipc_message(): Failed to lock queue semaphore while the message queue was not empty!\n" );
        }
    }

    if ( port->message_queue == NULL ) {
        error = -ENOENT;
        goto error2;
    }

    message = port->message_queue;

    if ( code != NULL ) {
        *code = message->code;
    }

    if ( size != NULL ) {
        *size = message->size;
    }

    semaphore_unlock( port->queue_semaphore, 1 );

    mutex_unlock( ipc_port_mutex );

    return 0;

 error2:
    mutex_unlock( ipc_port_mutex );

 error1:
    return error;
}

int sys_register_named_ipc_port( const char* name, ipc_port_id port_id ) {
    int error;
    size_t name_length;
    named_ipc_port_t* port;

    mutex_lock( named_ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    port = ( named_ipc_port_t* )hashtable_get( &named_ipc_port_table, ( const void* )name );

    if ( port != NULL ) {
        error = -EEXIST;
        port = NULL; /* This is needed, so the error handler won't destroy the port. */
        goto error;
    }

    name_length = strlen( name );

    port = ( named_ipc_port_t* )kmalloc( sizeof( named_ipc_port_t ) + name_length + 1 );

    if ( port == NULL ) {
        error = -ENOMEM;
        goto error;
    }

    port->port_id = port_id;
    port->name = ( char* )( port + 1 );
    strcpy( port->name, name );

    error = hashtable_add( &named_ipc_port_table, ( hashitem_t* )port );

    if ( error < 0 ) {
        goto error;
    }

    mutex_unlock( named_ipc_port_mutex );

    return 0;

 error:
    mutex_unlock( named_ipc_port_mutex );

    if ( port != NULL ) {
        kfree( port );
    }

    return error;
}

int sys_get_named_ipc_port( const char* name, ipc_port_id* port_id ) {
    int error;
    named_ipc_port_t* port;

    mutex_lock( named_ipc_port_mutex, LOCK_IGNORE_SIGNAL );

    port = ( named_ipc_port_t* )hashtable_get( &named_ipc_port_table, ( const void* )name );

    if ( port != NULL ) {
        *port_id = port->port_id;

        error = 0;
    } else {
        error = -ENOENT;
    }

    mutex_unlock( named_ipc_port_mutex );

    return error;
}

static void* ipc_port_key( hashitem_t* item ) {
    ipc_port_t* port;

    port = ( ipc_port_t* )item;

    return ( void* )&port->id;
}

static void* named_ipc_port_key( hashitem_t* item ) {
    named_ipc_port_t* port;

    port = ( named_ipc_port_t* )item;

    return ( void* )port->name;
}

__init int init_ipc( void ) {
    int error;

    error = init_hashtable(
        &ipc_port_table,
        64,
        ipc_port_key,
        hash_int,
        compare_int
    );

    if ( error < 0 ) {
        goto error1;
    }

    ipc_port_mutex = mutex_create( "IPC port table mutex", MUTEX_NONE );

    if ( ipc_port_mutex < 0 ) {
        error = ipc_port_mutex;
        goto error2;
    }

    error = init_hashtable(
        &named_ipc_port_table,
        8,
        named_ipc_port_key,
        hash_str,
        compare_str
    );

    if ( error < 0 ) {
        goto error3;
    }

    named_ipc_port_mutex = mutex_create( "Named IPC port table mutex", MUTEX_NONE );

    if ( named_ipc_port_mutex < 0 ) {
        error = named_ipc_port_mutex;
        goto error4;
    }

    ipc_port_id_counter = 0;

    return 0;

error4:
    destroy_hashtable( &named_ipc_port_table );

error3:
    mutex_destroy( ipc_port_mutex );

error2:
    destroy_hashtable( &ipc_port_table );

error1:
    return error;
}
