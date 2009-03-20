/* IRC client
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

#include <string.h>
#include <sys/select.h>
#include <yutil/array.h>

#include "eventmanager.h"

#include <yaosp/debug.h>

static volatile int running = 1;

static array_t event_list;

int event_manager_add_event( event_t* event ) {
    array_add_item( &event_list, ( void* )event );

    return 0;
}

int event_manager_mainloop( void ) {
    int i;
    int j;
    int count;
    int max_fd;
    event_t* event;

    fd_set read_set;
    fd_set write_set;
    fd_set except_set;

    fd_set* fd_set_table[ EVENT_COUNT ] = {
        &read_set,
        &write_set,
        &except_set
    };

    while ( running ) {
        /* Clear all fd_set */

        for ( i = 0; i < EVENT_COUNT; i++ ) {
            FD_ZERO( fd_set_table[ i ] );
        }

        max_fd = 0;

        /* Add fds to the sets */

        count = array_get_size( &event_list );

        for ( i = 0; i < count; i++ ) {
            event = ( event_t* )array_get_item( &event_list, i );

            for ( j = 0; j < EVENT_COUNT; j++ ) {
                if ( event->events[ j ].interested ) {
                    if ( event->fd > max_fd ) {
                        max_fd = event->fd;
                    }

                    FD_SET( event->fd, fd_set_table[ j ] );
                }
            }
        }

        /* Call select */

        count = select( max_fd + 1, &read_set, &write_set, &except_set, NULL );

        if ( count > 0 ) {
            count = array_get_size( &event_list );

            for ( i = 0; i < count; i++ ) {
                event = ( event_t* )array_get_item( &event_list, i );

                for ( j = 0; j < EVENT_COUNT; j++ ) {
                    if ( ( event->events[ j ].interested ) &&
                         ( FD_ISSET( event->fd, fd_set_table[ j ] ) ) ) {
                        event->events[ j ].callback( event );
                    }
                }
            }
        }
    }

    return 0;
}

int event_manager_quit( void ) {
    running = 0;

    return 0;
}

int init_event_manager( void ) {
    int error;

    error = init_array( &event_list );

    if ( error < 0 ) {
        return error;
    }

    array_set_realloc_size( &event_list, 32 );

    return 0;
}
