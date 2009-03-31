/* GUI server
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

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <yaosp/debug.h>
#include <yaosp/semaphore.h>
#include <yaosp/thread.h>

#include <input.h>
#include <mouse.h>

static int free_event_count;
static semaphore_id free_list_lock;
static input_event_t* first_free_event;

static input_event_t* event_queue;
static input_event_t* event_queue_tail;
static semaphore_id event_queue_lock;
static semaphore_id event_queue_sync;

static thread_id input_thread;

static input_driver_t* input_drivers[] = {
    &ps2mouse_driver,
    NULL
};

input_event_t* get_input_event( input_event_type_t type, int param1, int param2 ) {
    input_event_t* event;

    LOCK( free_list_lock );

    event = first_free_event;

    if ( event != NULL ) {
        assert( free_event_count > 0 );

        first_free_event = event->next;

        free_event_count--;
    }

    UNLOCK( free_list_lock );

    if ( event == NULL ) {
        event = ( input_event_t* )malloc( sizeof( input_event_t ) );

        if ( event == NULL ) {
            return NULL;
        }
    }

    event->type = type;
    event->param1 = param1;
    event->param2 = param2;

    return event;
}

int put_input_event( input_event_t* event ) {
    LOCK( free_list_lock );

    if ( free_event_count < MAX_FREE_EVENT_COUNT ) {
        event->next = first_free_event;
        first_free_event = event;

        free_event_count++;

        event = NULL;
    }

    UNLOCK( free_list_lock );

    if ( event != NULL ) {
        free( event );
    }

    return 0;
}

int insert_input_event( input_event_t* event ) {
    event->next = NULL;

    LOCK( event_queue_lock );

    if ( event_queue == NULL ) {
        assert( event_queue_tail == NULL );

        event_queue = event;
        event_queue_tail = event;
    } else {
        event_queue_tail->next = event;
        event_queue_tail = event;
    }

    UNLOCK( event_queue_lock );

    UNLOCK( event_queue_sync );

    return 0;
}

static int input_thread_entry( void* arg ) {
    input_event_t* event;

    while ( 1 ) {
        LOCK( event_queue_sync );

        LOCK( event_queue_lock );

        event = event_queue;

        if ( event != NULL ) {
            event_queue = event->next;

            if ( event_queue == NULL ) {
                event_queue_tail = NULL;
            }
        }

        UNLOCK( event_queue_lock );

        if ( event != NULL ) {
            switch ( event->type ) {
                case MOUSE_MOVED :
                    mouse_moved( event->param1, event->param2 );
                    break;
            }

            put_input_event( event );
        }
    }

    return 0;
}

int init_input_system( void ) {
    int i;
    int error;
    input_event_t* event;
    input_driver_t* input_driver;

    free_list_lock = create_semaphore( "Input free list", SEMAPHORE_BINARY, 0, 1 );

    if ( free_list_lock < 0 ) {
        return free_list_lock;
    }

    event_queue_lock = create_semaphore( "Input event queue lock", SEMAPHORE_BINARY, 0, 1 );

    if ( event_queue_lock < 0 ) {
        return event_queue_lock;
    }

    event_queue_sync = create_semaphore( "Input event queue sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( event_queue_sync < 0 ) {
        return event_queue_sync;
    }

    first_free_event = NULL;

    for ( i = 0; i < INIT_FREE_EVENT_COUNT; i++ ) {
        event = ( input_event_t* )malloc( sizeof( input_event_t ) );

        if ( event == NULL ) {
            return -ENOMEM;
        }

        event->next = first_free_event;
        first_free_event = event;
    }

    free_event_count = INIT_FREE_EVENT_COUNT;

    event_queue = NULL;
    event_queue_tail = NULL;

    input_thread = create_thread(
        "input dispatcher",
        PRIORITY_DISPLAY,
        input_thread_entry,
        NULL,
        0
    );

    if ( input_thread < 0 ) {
        return input_thread;
    }

    wake_up_thread( input_thread );

    for ( i = 0; input_drivers[ i ] != NULL; i++ ) {
        input_driver = input_drivers[ i ];

        error = input_driver->init();

        if ( error < 0 ) {
            dbprintf( "Failed to initialize input driver: %s\n", input_driver->name );
            continue;
        }

        error = input_driver->start();

        if ( error < 0 ) {
            dbprintf( "Failed to start input driver: %s\n", input_driver->name );
            continue;
        }

        dbprintf( "Input driver %s started.\n", input_driver->name );
    }

    return 0;
}
