/* Terminal driver
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

#include <input.h>
#include <ioctl.h>
#include <macros.h>
#include <vfs/vfs.h>

#include "terminal.h"

static int input_device = -1;
static int current_qualifiers = 0;

static inline int terminal_send_event( terminal_t* terminal, const char* data, size_t size ) {
    int error;

    if ( ( terminal->flags & TERMINAL_ACCEPTS_USER_INPUT ) == 0 ) {
        return 0;
    }

    UNLOCK( terminal_lock );

    error = pwrite(
        terminal->master_pty,
        data,
        size,
        0
    );

    LOCK( terminal_lock );

    if ( error != ( int )size ) {
        return -1;
    }

    return 0;
}

static int terminal_handle_event( input_event_t* event ) {
    LOCK( terminal_lock );

    switch ( ( int )event->event ) {
        case E_KEY_PRESSED : {
            int tmp;

            /* Scroll the terminal to the bottom */

            tmp = MAX( 0, active_terminal->line_count - TERMINAL_HEIGHT );

            if ( active_terminal->start_line != tmp ) {
                active_terminal->start_line = tmp;

                terminal_do_full_update( active_terminal );
            }

            /* Write the new character to the terminal */

            switch ( event->param1 ) {
                case KEY_LEFT :
                    terminal_send_event( active_terminal, "\x1b[D", 3 );
                    break;

                case KEY_RIGHT :
                    terminal_send_event( active_terminal, "\x1b[C", 3 );
                    break;

                case KEY_UP :
                    terminal_send_event( active_terminal, "\x1b[A", 3 );
                    break;

                case KEY_DOWN :
                    terminal_send_event( active_terminal, "\x1b[B", 3 );
                    break;

                case KEY_HOME :
                    terminal_send_event( active_terminal, "\x1b[H", 3 );
                    break;

                case KEY_END :
                    terminal_send_event( active_terminal, "\x1b[F", 3 );
                    break;

                case KEY_DELETE :
                    terminal_send_event( active_terminal, "\x1b[3~", 4 );
                    break;

                case KEY_PAGE_UP :
                    if ( current_qualifiers & Q_SHIFT ) {
                        terminal_scroll( -10 );
                    }

                    break;

                case KEY_PAGE_DOWN :
                    if ( current_qualifiers & Q_SHIFT ) {
                        terminal_scroll( 10 );
                    }

                    break;

                case KEY_F1 :
                case KEY_F2 :
                case KEY_F3 :
                case KEY_F4 :
                case KEY_F5 :
                case KEY_F6 :
                    if ( current_qualifiers & Q_ALT ) {
                        terminal_switch_to( ( event->param1 - KEY_F1 ) >> 8 );
                    }

                    break;

                default :
                    terminal_send_event( active_terminal, ( const char* )&event->param1, 1 );
                    break;
            }

            break;
        }

        case E_KEY_RELEASED :
            break;

        case E_QUALIFIERS_CHANGED :
            current_qualifiers = event->param1;
            break;

        default :
            kprintf( "Terminal: Unknown input event: %d\n", event->event );
            break;
    }

    UNLOCK( terminal_lock );

    return 0;
}

static int terminal_input_thread( void* arg ) {
    int error;
    input_event_t event;

    if ( input_device < 0 ) {
        return 0;
    }

    while ( 1 ) {
        error = pread( input_device, &event, sizeof( input_event_t ), 0 );

        if ( __unlikely( error < 0 ) ) {
            kprintf( "Failed to read from input node: %d\n", error );
            break;
        }

        terminal_handle_event( &event );
    }

    close( input_device );

    return 0;
}

int init_terminal_input( void ) {
    int fd;
    int error;
    char path[ 64 ];
    thread_id thread;
    input_cmd_create_node_t cmd;

    fd = open( "/device/control/input", O_RDONLY );

    if ( fd < 0 ) {
        return fd;
    }

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

    thread = create_kernel_thread( "terminal_input", PRIORITY_NORMAL, terminal_input_thread, NULL, 0 );

    if ( thread < 0 ) {
        close( input_device );
        return thread;
    }

    wake_up_thread( thread );

    return 0;
}
