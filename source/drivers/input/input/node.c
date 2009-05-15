/* Input driver node
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

#include <types.h>
#include <semaphore.h>
#include <errno.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/stack.h>
#include <lib/string.h>

#include "input.h"

static stack_t input_stack;
static semaphore_id input_stack_lock = -1;
static uint32_t input_node_counter = 0;

static input_device_t* active_input_receiver = NULL;

static int input_device_open( void* node, uint32_t flags, void** cookie ) {
    input_device_t* device;

    device = ( input_device_t* )node;

    set_input_driver_states( &device->input_state );

    return 0;
}

static int input_device_close( void* node, void* cookie ) {
    return 0;
}

static int input_device_read( void* node, void* cookie, void* buffer, off_t position, size_t size ) {
    input_device_t* device;
    input_event_wrapper_t* wrapper;

    if ( size < sizeof( input_event_t ) ) {
        return -E2BIG;
    }

    device = ( input_device_t* )node;

    LOCK( device->lock );

    while ( device->first_event == NULL ) {
        UNLOCK( device->lock );
        LOCK( device->sync );
        LOCK( device->lock );
    }

    wrapper = device->first_event;
    device->first_event = wrapper->next;

    if ( device->first_event == NULL ) {
        device->last_event = NULL;
    }

    UNLOCK( device->lock );

    memcpy( buffer, &wrapper->e, sizeof( input_event_t ) );

    kfree( wrapper );

    return 0;
}

static device_calls_t input_device_calls = {
    .open = input_device_open,
    .close = input_device_close,
    .ioctl = NULL,
    .read = input_device_read,
    .write = NULL,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

input_device_t* create_input_device( int flags ) {
    int error;
    input_device_t* device;

    device = ( input_device_t* )kmalloc( sizeof( input_device_t ) );

    if ( __unlikely( device == NULL ) ) {
        error = -ENOMEM;
        goto error1;
    }

    device->lock = create_semaphore( "input node lock", SEMAPHORE_BINARY, 0, 1 );

    if ( device->lock < 0 ) {
        error = device->lock;
        goto error2;
    }

    device->sync = create_semaphore( "input node sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( device->sync < 0 ) {
        error = device->sync;
        goto error3;
    }

    error = init_input_driver_states( &device->input_state );

    if ( __unlikely( error < 0 ) ) {
        goto error4;
    }

    device->flags = flags;
    device->first_event = NULL;
    device->last_event = NULL;

    return device;

error4:
    delete_semaphore( device->sync );

error3:
    delete_semaphore( device->lock );

error2:
    kfree( device );

error1:
    return NULL;
}

void destroy_input_device( input_device_t* device ) {
    /* TODO: Delete the node from /device/input/input... */

    kfree( device );
}

int insert_input_device( input_device_t* device ) {
    int error;
    void* dummy;
    char node[ 64 ];

    error = LOCK( input_stack_lock );

    if ( __unlikely( error < 0 ) ) {
        return error;
    }

    error = stack_push( &input_stack, ( void* )device );

    if ( __unlikely( error < 0 ) ) {
        goto error1;
    }

    snprintf( node, sizeof( node ), "input/node/%u", input_node_counter );

    input_node_counter++;

    error = create_device_node( node, &input_device_calls, ( void* )device );

    if ( error < 0 ) {
        goto error2;
    }

    active_input_receiver = device;

    UNLOCK( input_stack_lock );

    return 0;

error2:
    stack_pop( &input_stack, &dummy );

error1:
    UNLOCK( input_stack_lock );

    return error;
}

int insert_input_event( input_event_t* event ) {
    int error;
    input_event_wrapper_t* wrapper;

    LOCK( input_stack_lock );

    if ( active_input_receiver != NULL ) {
        error = LOCK( active_input_receiver->lock );

        if ( error < 0 ) {
            goto out;
        }

        wrapper = ( input_event_wrapper_t* )kmalloc( sizeof( input_event_wrapper_t ) );

        ASSERT( wrapper != NULL ); /* ... this is not a good idea actually */

        memcpy( &wrapper->e, event, sizeof( input_event_t ) );
        wrapper->next = NULL;

        if ( active_input_receiver->first_event == NULL ) {
            active_input_receiver->first_event = wrapper;
            active_input_receiver->last_event = wrapper;
        } else {
            active_input_receiver->last_event->next = wrapper;
            active_input_receiver->last_event = wrapper;
        }

        UNLOCK( active_input_receiver->lock );

        /* Tell the readers that they have something to read ;) */

        UNLOCK( active_input_receiver->sync );
    }

 out:
    UNLOCK( input_stack_lock );

    return 0;
}

int init_node_manager( void ) {
    int error;

    error = init_stack( &input_stack );

    if ( error < 0 ) {
        goto error1;
    }

    input_stack_lock = create_semaphore( "input stack lock", SEMAPHORE_BINARY, 0, 1 );

    if ( input_stack_lock < 0 ) {
        error = input_stack_lock;
        goto error2;
    }

    return 0;

error2:
    destroy_stack( &input_stack );

error1:
    return error;
}
