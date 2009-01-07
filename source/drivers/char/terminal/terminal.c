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

#include <console.h>
#include <errno.h>
#include <thread.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include "pty.h"
#include "terminal.h"
#include "kterm.h"
#include "input.h"

semaphore_id lock;
thread_id read_thread;
terminal_t* active_terminal = NULL;
terminal_t* terminals[ MAX_TERMINAL_COUNT ];

static console_t* screen;

static terminal_input_t* input_drivers[] = {
    &ps2_keyboard
};

int terminal_handle_event( event_type_t event, int param1, int param2 ) {
    LOCK( lock );

    switch ( ( int )event ) {
        case E_KEY_PRESSED :
            if ( active_terminal->flags & TERMINAL_ACCEPTS_USER_INPUT ) {
                pwrite( active_terminal->master_pty, &param1, 1, 0 );
            }

            break;
    }

    UNLOCK( lock );

    return 0;
}

int terminal_switch_to( int index ) {
    int i;
    int j;
    term_buffer_item_t* line;

    if ( ( index < 0 ) || ( index >= MAX_TERMINAL_COUNT ) ) {
        return -EINVAL;
    }

    LOCK( lock );

    active_terminal = terminals[ index ];

    /* Clear the screen */

    screen->ops->clear( screen );

    /* Put the terminal buffer to the screen */

    i = MAX( 0, active_terminal->line_count - TERMINAL_HEIGHT );

    for ( ; i < active_terminal->line_count; i++ ) {
        line = &active_terminal->lines[ i ];

        for ( j = 0; j < line->size; j++ ) {
            screen->ops->putchar( screen, line->buffer[ j ] );
        }

        if ( ( line->size < TERMINAL_WIDTH ) &&
             ( line->flags & TERM_BUFFER_LINE_END ) ) {
            screen->ops->putchar( screen, '\n' );
        }
    }

    UNLOCK( lock );

    return 0;
}

static int terminal_buffer_insert( terminal_t* terminal, char* buf, int size ) {
    bool done;

    /* Check if we can insert to the last line */

    if ( ( terminal->line_count > 0 ) &&
         ( ( terminal->lines[ terminal->line_count - 1 ].flags & TERM_BUFFER_LINE_END ) == 0 ) &&
         ( terminal->lines[ terminal->line_count - 1 ].size < TERMINAL_WIDTH ) ) {
        term_buffer_item_t* last_line;

        done = false;
        last_line = &terminal->lines[ terminal->line_count - 1 ];

        while ( ( size > 0 ) && ( last_line->size < TERMINAL_WIDTH ) && ( !done ) ) {
            switch ( *buf ) {
                case '\n' :
                    last_line->flags |= TERM_BUFFER_LINE_END;
                    done = true;
                    break;

                default :
                    last_line->buffer[ last_line->size++ ] = *buf;
                    break;
            }

            buf++;
            size--;
        }

        last_line->buffer[ last_line->size ] = 0;
    }

    /* Create new line(s) for the rest of the data */

    while ( size > 0 ) {
        char* buffer;
        term_buffer_item_t* last_line;

        if ( terminal->line_count == TERMINAL_MAX_LINES ) {
            buffer = terminal->lines[ 0 ].buffer;
            memmove( terminal->lines, &terminal->lines[ 1 ], sizeof( term_buffer_item_t ) * ( TERMINAL_MAX_LINES - 1 ) );
        } else {
            terminal->line_count++;

            buffer = ( char* )kmalloc( TERMINAL_WIDTH + 1 );

            if ( buffer == NULL ) {
                return -ENOMEM;
            }
        }

        last_line = &terminal->lines[ terminal->line_count - 1 ];

        last_line->size = 0;
        last_line->flags = 0;
        last_line->buffer = buffer;

        done = false;

        while ( ( size > 0 ) && ( last_line->size < TERMINAL_WIDTH ) && ( !done ) ) {
            switch ( *buf ) {
                case '\n' :
                    last_line->flags |= TERM_BUFFER_LINE_END;
                    done = true;
                    break;

                default :
                    last_line->buffer[ last_line->size ] = *buf;
                    last_line->size++;
                    break;
            }

            buf++;
            size--;
        }

        last_line->buffer[ last_line->size ] = 0;
    }

    return 0;
}

static int terminal_read_thread( void* arg ) {
    int i;
    int j;
    int count;
    int max_fd;
    fd_set read_set;
    int size;
    char buffer[ 512 ];

    while ( 1 ) {
        max_fd = -1;
        FD_ZERO( &read_set );

        for ( i = 0; i < MAX_TERMINAL_COUNT; i++ ) {
            FD_SET( terminals[ i ]->master_pty, &read_set );

            if ( terminals[ i ]->master_pty > max_fd ) {
                max_fd = terminals[ i ]->master_pty;
            }
        }

        count = select( max_fd + 1, &read_set, NULL, NULL, NULL );

        if ( count < 0 ) {
            kprintf( "Terminal: Select error: %d\n", count );
            continue;
        }

        for ( i = 0; i < MAX_TERMINAL_COUNT; i++ ) {
            if ( FD_ISSET( terminals[ i ]->master_pty, &read_set ) ) {
                size = pread( terminals[ i ]->master_pty, buffer, sizeof( buffer ), 0 );

                if ( size > 0 ) {
                    LOCK( lock );

                    terminal_buffer_insert( terminals[ i ], buffer, size );

                    if ( active_terminal == terminals[ i ] ) {
                        for ( j = 0; j < size; j++ ) {
                            screen->ops->putchar( screen, buffer[ j ] );
                        }
                    }

                    UNLOCK( lock );
                }
            }
        }
    }

    return 0;
}

int init_terminals( void ) {
    int i;
    char path[ 128 ];

    for ( i = 0; i < MAX_TERMINAL_COUNT; i++ ) {
        terminals[ i ] = ( terminal_t* )kmalloc( sizeof( terminal_t ) );

        if ( terminals[ i ] == NULL ) {
            return -ENOMEM;
        }

        snprintf( path, sizeof( path ), "/device/pty/pty%d", i );

        terminals[ i ]->master_pty = open( path, O_RDWR | O_CREAT );

        if ( terminals[ i ]->master_pty < 0 ) {
            return terminals[ i ]->master_pty;
        }

        terminals[ i ]->lines = ( term_buffer_item_t* )kmalloc( sizeof( term_buffer_item_t ) * TERMINAL_MAX_LINES );

        if ( terminals[ i ]->lines == NULL ) {
            return -ENOMEM;
        }

        memset( terminals[ i ]->lines, 0, sizeof( term_buffer_item_t ) * TERMINAL_MAX_LINES );

        terminals[ i ]->flags = TERMINAL_ACCEPTS_USER_INPUT;
        terminals[ i ]->line_count = 0;
    }

    read_thread = create_kernel_thread( "terminal read", terminal_read_thread, NULL );

    if ( read_thread < 0 ) {
        return read_thread;
    }

    wake_up_thread( read_thread );

    return 0;
}

int init_module( void ) {
    int i;
    int error;

    lock = create_semaphore( "terminal lock", SEMAPHORE_BINARY, 0, 1 );

    if ( lock < 0 ) {
        return lock;
    }

    error = mkdir( "/device/pty", 0 );

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to create /device/pty\n" );
        return error;
    }

    error = init_pty_filesystem();

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to initialize pty filesystem\n" );
        return error;
    }

    error = mount( "", "/device/pty", "pty" );

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to mount pty filesystem!\n" );
        return error;
    }

    error = init_terminals();

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to initialize terminals!\n" );
        return error;
    }

    error = console_switch_screen( NULL, &screen );

    if ( error < 0 ) {
        return error;
    }

    error = init_kernel_terminal();

    if ( error < 0 ) {
        return error;
    }

    /* Initialize inputs */

    for ( i = 0; i < ARRAY_SIZE( input_drivers ); i++ ) {
        if ( input_drivers[ i ]->init() >= 0 ) {
            input_drivers[ i ]->start();
        }
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
