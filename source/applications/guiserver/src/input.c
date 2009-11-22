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
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <yaosp/debug.h>
#include <yaosp/input.h>

#include <input.h>
#include <windowmanager.h>

static int input_device = -1;
static pthread_t input_thread;

static void* input_thread_entry( void* arg ) {
    int error;
    input_event_t event;

    assert( input_device >= 0 );

    while ( 1 ) {
        error = read( input_device, &event, sizeof( input_event_t ) );

        if ( error < 0 ) {
            dbprintf( "Failed to read from input node: %d\n", error );
            break;
        }

        switch ( event.event ) {
            case E_KEY_PRESSED :
                wm_key_pressed( event.param1 );
                break;

            case E_KEY_RELEASED :
                wm_key_released( event.param1 );
                break;

            case E_QUALIFIERS_CHANGED :
                break;

            case E_MOUSE_MOVED : {
                point_t delta = {
                    .x = event.param1,
                    .y = event.param2
                };

                wm_mouse_moved( &delta );

                break;
            }

            case E_MOUSE_PRESSED :
                wm_mouse_pressed( event.param1 );
                break;

            case E_MOUSE_RELEASED :
                wm_mouse_released( event.param1 );
                break;

            case E_MOUSE_SCROLLED :
                break;
        }
    }

    close( input_device );

    return NULL;
}

int input_system_start( void ) {
    pthread_create(
        &input_thread,
        NULL,
        input_thread_entry,
        NULL
    );

    return 0;
}

int init_input_system( void ) {
    int fd;
    int error;
    char path[ 128 ];
    input_cmd_create_node_t cmd;

    fd = open( "/device/control/input", O_RDONLY );

    if ( fd < 0 ) {
        return fd;
    }

    cmd.flags = INPUT_KEY_EVENTS | INPUT_MOUSE_EVENTS;

    error = ioctl( fd, IOCTL_INPUT_CREATE_DEVICE, ( void* )&cmd );

    close( fd );

    if ( error < 0 ) {
        return error;
    }

    snprintf( path, sizeof( path ), "/device/input/node/%u", cmd.node_number );

    input_device = open( path, O_RDONLY );

    if ( input_device < 0 ) {
        return input_device;
    }

    return 0;
}
