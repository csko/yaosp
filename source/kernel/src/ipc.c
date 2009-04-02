/* Inter Process Communication
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

#include <ipc.h>
#include <config.h>
#include <macros.h>
#include <errno.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

static int ipc_port_id_counter;
static hashtable_t ipc_port_table;
static semaphore_id ipc_port_lock;

static hashtable_t named_ipc_port_table;
static semaphore_id named_ipc_port_lock;

static int insert_ipc_port( ipc_port_t* port ) {
    int error;

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

ipc_port_id sys_create_ipc_port( void ) {
    int error;
    ipc_port_t* port;

    port = ( ipc_port_t* )kmalloc( sizeof( ipc_port_t ) );

    if ( port == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    port->queue_sync = create_semaphore( "IPC queue sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( port->queue_sync < 0 ) {
        error = port->queue_sync;
        goto error2;
    }

    port->queue_size = 0;
    port->message_queue = NULL;
    port->message_queue_tail = NULL;

    LOCK( ipc_port_lock );

    error = insert_ipc_port( port );

    UNLOCK( ipc_port_lock );

    if ( error < 0 ) {
        goto error3;
    }

    return port->id;

error3:
    delete_semaphore( port->queue_sync );

error2:
    kfree( port );

error1:
    return error;
}

int sys_send_ipc_message( ipc_port_id port_id, uint32_t code, void* data, size_t size ) {
    int error;
    ipc_port_t* port;
    ipc_message_t* message;

    LOCK( ipc_port_lock );

    port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

    if ( port == NULL ) {
        error = -EINVAL;
        goto error1;
    }

    if ( ( port->queue_size + size ) > MAX_IPC_MSG_QUEUE_SIZE ) {
        error = -E2BIG;
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

    UNLOCK( port->queue_sync );

    UNLOCK( ipc_port_lock );

    return 0;

error1:
    UNLOCK( ipc_port_lock );

    return error;
}

int sys_recv_ipc_message( ipc_port_id port_id, uint32_t* code, void* buffer, size_t size, uint64_t* timeout ) {
    int error;
    ipc_port_t* port;
    semaphore_id queue_sync;
    ipc_message_t* message;

    LOCK( ipc_port_lock );

    port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

    if ( port == NULL ) {
        error = -EINVAL;
        goto error2;
    }

    queue_sync = port->queue_sync;

    UNLOCK( ipc_port_lock );

    error = lock_semaphore( queue_sync, 1, *timeout );

    if ( error < 0 ) {
        goto error1;
    }

    LOCK( ipc_port_lock );

    port = ( ipc_port_t* )hashtable_get( &ipc_port_table, ( const void* )&port_id );

    if ( port == NULL ) {
        error = -EINVAL;
        goto error2;
    }

    if ( port->message_queue == NULL ) {
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

    UNLOCK( ipc_port_lock );

    *code = message->code;

    if ( message->size > 0 ) {
        ASSERT( message->size <= size );

        memcpy( buffer, ( void* )( message + 1 ), message->size );
    }

    error = message->size;

    kfree( message );

    return error;

error2:
    UNLOCK( ipc_port_lock );

error1:
    return error;
}

int sys_register_named_ipc_port( const char* name, ipc_port_id port_id ) {
    int error;
    named_ipc_port_t* port;

    LOCK( named_ipc_port_lock );

    port = ( named_ipc_port_t* )hashtable_get( &named_ipc_port_table, ( const void* )name );

    if ( port != NULL ) {
        error = -EEXIST;
        goto error1;
    }

    port = ( named_ipc_port_t* )kmalloc( sizeof( named_ipc_port_t ) );

    if ( port == NULL ) {
        error = -ENOMEM;
        goto error1;
    }

    port->name = strdup( name );

    if ( port->name == NULL ) {
        error = -ENOMEM;
        goto error2;
    }

    port->port_id = port_id;

    error = hashtable_add( &named_ipc_port_table, ( hashitem_t* )port );

    if ( error < 0 ) {
        goto error3;
    }

    UNLOCK( named_ipc_port_lock );

    return 0;

error3:
    kfree( ( void* )port->name );

error2:
    kfree( port );

error1:
    UNLOCK( named_ipc_port_lock );

    return error;
}

int sys_get_named_ipc_port( const char* name, ipc_port_id* port_id ) {
    int error;
    named_ipc_port_t* port;

    LOCK( named_ipc_port_lock );

    port = ( named_ipc_port_t* )hashtable_get( &named_ipc_port_table, ( const void* )name );

    if ( port != NULL ) {
        *port_id = port->port_id;

        error = 0;
    } else {
        error = -ENOENT;
    }

    UNLOCK( named_ipc_port_lock );

    return error;
}

static void* ipc_port_key( hashitem_t* item ) {
    ipc_port_t* port;

    port = ( ipc_port_t* )item;

    return ( void* )&port->id;
}

static uint32_t ipc_port_hash( const void* key ) {
    return hash_number( ( uint8_t* )key, sizeof( ipc_port_id ) );
}

static bool ipc_port_compare( const void* key1, const void* key2 ) {
    ipc_port_id id1;
    ipc_port_id id2;

    id1 = *( ( ipc_port_id* )key1 );
    id2 = *( ( ipc_port_id* )key2 );

    return ( id1 == id2 );
}

static void* named_ipc_port_key( hashitem_t* item ) {
    named_ipc_port_t* port;

    port = ( named_ipc_port_t* )item;

    return ( void* )port->name;
}

static uint32_t named_ipc_port_hash( const void* key ) {
    return hash_string( ( uint8_t* )key, strlen( ( const char* )key ) );
}

static bool named_ipc_port_compare( const void* key1, const void* key2 ) {
    return ( strcmp( ( const char* )key1, ( const char* )key2 ) == 0 );
}

__init int init_ipc( void ) {
    int error;

    error = init_hashtable(
        &ipc_port_table,
        64,
        ipc_port_key,
        ipc_port_hash,
        ipc_port_compare
    );

    if ( error < 0 ) {
        goto error1;
    }

    ipc_port_lock = create_semaphore( "IPC port table lock", SEMAPHORE_BINARY, 0, 1 );

    if ( ipc_port_lock < 0 ) {
        error = ipc_port_lock;
        goto error2;
    }

    error = init_hashtable(
        &named_ipc_port_table,
        8,
        named_ipc_port_key,
        named_ipc_port_hash,
        named_ipc_port_compare
    );

    if ( error < 0 ) {
        goto error3;
    }

    named_ipc_port_lock = create_semaphore( "Named IPC port table lock", SEMAPHORE_BINARY, 0, 1 );

    if ( named_ipc_port_lock < 0 ) {
        goto error4;
    }

    ipc_port_id_counter = 0;

    return 0;

error4:
    destroy_hashtable( &named_ipc_port_table );

error3:
    delete_semaphore( ipc_port_lock );

error2:
    destroy_hashtable( &ipc_port_table );

error1:
    return error;
}
