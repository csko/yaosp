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
#include <module.h>
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

static void terminal_do_full_update( terminal_t* terminal ) {
    int i;
    int j;
    char bg_color;
    char fg_color;
    int lines_to_write;
    terminal_buffer_t* buffer;
    terminal_buffer_item_t* data_item;

    /* Clear the screen */

    screen->ops->set_bg_color( screen, COLOR_BLACK );
    screen->ops->set_fg_color( screen, COLOR_LIGHT_GRAY );
    screen->ops->clear( screen );

    bg_color = COLOR_BLACK;
    fg_color = COLOR_LIGHT_GRAY;

    ASSERT( terminal->line_count > 0 );

    lines_to_write = MIN( screen->height, terminal->line_count - terminal->start_line );

    buffer = &terminal->lines[ terminal->start_line ];

    for ( i = 0; ( i < lines_to_write ) && ( buffer->data != NULL ); i++, buffer++ ) {
        data_item = &buffer->data[ 0 ];

        for ( j = 0; j < buffer->last_dirty; j++, data_item++ ) {
            /* Check BG color */

            if ( data_item->bg_color != bg_color ) {
                bg_color = data_item->bg_color;

                screen->ops->set_bg_color( screen, bg_color );
            }

            /* Check FG color */

            if ( data_item->fg_color != fg_color ) {
                fg_color = data_item->fg_color;

                screen->ops->set_fg_color( screen, fg_color );
            }

            /* Put the character */

            screen->ops->putchar( screen, data_item->character );
        }

        /* Put a newline if the current line is not the last one and we didn't print a whole line */

        if ( ( buffer->last_dirty != ( screen->width - 1 ) ) && ( i != ( lines_to_write - 1 ) ) ) {
            screen->ops->putchar( screen, '\n' );
        }
    }

    /* Move the cursor to the right position */

    /* TODO: do this only if it's a valid position on the real screen */
    screen->ops->gotoxy( screen, terminal->cursor_column, terminal->cursor_row - terminal->start_line );
}

int terminal_handle_event( event_type_t event, int param1, int param2 ) {
    LOCK( lock );

    switch ( ( int )event ) {
        case E_KEY_PRESSED : {
            int tmp;

            /* Scroll the terminal to the bottom */

            tmp = MAX( 0, active_terminal->line_count - TERMINAL_HEIGHT );

            if ( active_terminal->start_line != tmp ) {
                active_terminal->start_line = tmp;

                terminal_do_full_update( active_terminal );
            }

            /* Write the new character to the terminal */

            if ( active_terminal->flags & TERMINAL_ACCEPTS_USER_INPUT ) {
                pwrite( active_terminal->master_pty, &param1, 1, 0 );
            }

            break;
        }
    }

    UNLOCK( lock );

    return 0;
}

int terminal_scroll( int offset ) {
    int tmp;
    int new_start_line;

    LOCK( lock );

    new_start_line = active_terminal->start_line + offset;

    if ( new_start_line < 0 ) {
        new_start_line = 0;
    }

    tmp = MAX( 0, active_terminal->line_count - TERMINAL_HEIGHT );

    if ( new_start_line > tmp ) {
        new_start_line = tmp;
    }

    if ( active_terminal->start_line != new_start_line ) {
        active_terminal->start_line = new_start_line;

        terminal_do_full_update( active_terminal );
    }

    UNLOCK( lock );

    return 0;
}

int terminal_switch_to( int index ) {
    if ( ( index < 0 ) || ( index >= MAX_TERMINAL_COUNT ) ) {
        return -EINVAL;
    }

    LOCK( lock );

    /* Make sure we're switching to another terminal */

    if ( terminals[ index ] == active_terminal ) {
        UNLOCK( lock );

        return 0;
    }

    active_terminal = terminals[ index ];

    terminal_do_full_update( active_terminal );

    UNLOCK( lock );

    return 0;
}

static inline void terminal_handle_new_line( terminal_t* terminal ) {
    terminal_buffer_t* tmp;

    terminal->cursor_column = 0;

    ASSERT( terminal->line_count <= TERMINAL_MAX_LINES );

    if ( terminal->line_count == TERMINAL_MAX_LINES ) {
        terminal_buffer_item_t* new_data;

        new_data = terminal->lines[ 0 ].data;

        memmove(
            &terminal->lines[ 0 ],
            &terminal->lines[ 1 ],
            sizeof( terminal_buffer_t ) * ( TERMINAL_MAX_LINES - 1 )
        );

        tmp = &terminal->lines[ TERMINAL_MAX_LINES - 1 ];

        tmp->data = new_data;
        tmp->last_dirty = 0;
    } else {
        terminal->line_count++;

        /* Update start line if needed */

        if ( terminal->line_count > screen->height ) {
            terminal->start_line++;
        }
    }

    if ( terminal->cursor_row < ( terminal->line_count - 1 ) ) {
        terminal->cursor_row++;
    }
}

void terminal_put_char( terminal_t* terminal, char c ) {
    terminal_buffer_t* buffer;
    terminal_buffer_item_t* data_item;

    /* Check if the current line has an allocated buffer */

    buffer = &terminal->lines[ terminal->cursor_row ];

    if ( buffer->data == NULL ) {
        int i;

        buffer->data = ( terminal_buffer_item_t* )kmalloc( sizeof( terminal_buffer_item_t ) * 80 );

        if ( buffer->data == NULL ) {
            return;
        }

        data_item = &buffer->data[ 0 ];

        for ( i = 0; i < 80; i++, data_item++ ) {
            data_item->character = ' ';
            data_item->bg_color = COLOR_BLACK;
            data_item->fg_color = COLOR_LIGHT_GRAY;
        }
    }

    if ( terminal == active_terminal ) {
        screen->ops->putchar( screen, c );
    }

    if ( c == '\n' ) {
        terminal_handle_new_line( terminal );
        return;
    }

    data_item = &buffer->data[ terminal->cursor_column ];

    data_item->character = c;
    data_item->fg_color = ( char )terminal->fg_color;
    data_item->bg_color = ( char )terminal->bg_color;

    if ( buffer->last_dirty < terminal->cursor_column ) {
        buffer->last_dirty = terminal->cursor_column;
    }

    terminal->cursor_column++;

    if ( terminal->cursor_column == 80 ) {
        terminal_handle_new_line( terminal );
    }
}

static inline void terminal_clear( terminal_t* terminal ) {
    if ( terminal == active_terminal ) {
        screen->ops->clear( screen );
    }
}

static inline void terminal_move_cursor_to( terminal_t* terminal, int cursor_x, int cursor_y ) {
    terminal->cursor_row = terminal->start_line + cursor_y;
    terminal->cursor_column = cursor_x;

    screen->ops->gotoxy( screen, cursor_x, cursor_y );
}

static inline console_color_t ansi_to_console_color( int color ) {
    switch ( color ) {
        case 0 : return COLOR_BLACK;
        case 1 : return COLOR_RED;
        case 2 : return COLOR_GREEN;
        case 3 : return COLOR_BLACK; /* TODO: yellow? */
        case 4 : return COLOR_BLUE;
        case 5 : return COLOR_MAGENTA;
        case 6 : return COLOR_CYAN;
        case 7 : return COLOR_WHITE;
        default : return COLOR_BLACK;
    }
}

static inline void terminal_set_fg_color( terminal_t* terminal, int ansi_color ) {
    console_color_t color;

    ASSERT( ( ansi_color >= 30 ) && ( ansi_color <= 37 ) );

    color = ansi_to_console_color( ansi_color - 30 );

    if ( terminal->fg_color != color ) {
        terminal->fg_color = color;

        if ( terminal == active_terminal ) {
            screen->ops->set_fg_color( screen, color );
        }
    }
}

static inline void terminal_set_bg_color( terminal_t* terminal, int ansi_color ) {
    console_color_t color;

    ASSERT( ( ansi_color >= 40 ) && ( ansi_color <= 47 ) );

    color = ansi_to_console_color( ansi_color - 40 );

    if ( terminal->bg_color != color ) {
        terminal->bg_color = color;

        if ( terminal == active_terminal ) {
            screen->ops->set_bg_color( screen, color );
        }
    }
}

static void terminal_parse_data( terminal_t* terminal, char* data, size_t size ) {
    int i;
    char c;

    for ( ; size > 0; size--, data++ ) {
        c = *data;

        switch ( terminal->input_state ) {
            case IS_NONE :
                switch ( c ) {
                    case 0x1B :
                        terminal->input_state = IS_ESC;
                        terminal->input_param_count = 0;
                        break;

                    default :
                        terminal_put_char( terminal, c );
                        break;
                }

                break;

            case IS_ESC :
                switch ( c ) {
                    case '[' :
                        terminal->input_state = IS_BRACKET;
                        break;

                    default :
                        terminal->input_state = IS_NONE;
                        break;
                }

                break;

            case IS_BRACKET :
                switch ( c ) {
                    case 's' :
                        /* TODO: Save cursor position */
                        terminal->input_state = IS_NONE;
                        break;

                    case 'u' :
                        /* TODO: Restore cursor position */
                        terminal->input_state = IS_NONE;
                        break;

                    case 'H' :
                    case 'f' : {
                        int new_x;
                        int new_y;

                        if ( terminal->input_param_count == 2 ) {
                            new_y = terminal->input_params[ 0 ];
                            new_x = terminal->input_params[ 1 ];
                        } else {
                            new_x = 0;
                            new_y = 0;
                        }

                        terminal_move_cursor_to( terminal, new_x, new_y );

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'm' :
                        for ( i = 0; i < terminal->input_param_count; i++ ) {
                            switch ( terminal->input_params[ i ] ) {
                                case 30 ... 37 :
                                    terminal_set_fg_color( terminal, terminal->input_params[ i ] );
                                    break;

                                case 40 ... 47 :
                                    terminal_set_bg_color( terminal, terminal->input_params[ i ] );
                                    break;
                            }

                            terminal->input_state = IS_NONE;
                        }

                        break;

                    case 'J' :
                        if ( ( terminal->input_param_count == 1 ) &&
                             ( terminal->input_params[ 0 ] == 2 ) ) {
                            terminal_clear( terminal );
                        }

                        terminal->input_state = IS_NONE;

                        break;

                    case '0' ... '9' :
                        if ( terminal->input_param_count == 0 ) {
                            terminal->input_param_count++;
                            terminal->input_params[ 0 ] = ( c - '0' );
                        } else {
                            terminal->input_params[ terminal->input_param_count - 1 ] *= 10;
                            terminal->input_params[ terminal->input_param_count - 1 ] += ( c - '0' );
                        }
                        break;

                    case ';' :
                        terminal->input_param_count++;
                        terminal->input_params[ terminal->input_param_count - 1 ] = 0;
                        break;

                    default :
                        terminal->input_state = IS_NONE;
                        break;
                }

                break;
        }
    }
}

static int terminal_read_thread( void* arg ) {
    int i;
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
                    terminal_parse_data( terminals[ i ], buffer, size );
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
    terminal_t* terminal;

    for ( i = 0; i < MAX_TERMINAL_COUNT; i++ ) {
        terminal = ( terminal_t* )kmalloc( sizeof( terminal_t ) );

        if ( terminal == NULL ) {
            return -ENOMEM;
        }

        snprintf( path, sizeof( path ), "/device/terminal/pty%d", i );

        terminal->master_pty = open( path, O_RDWR | O_CREAT );

        if ( terminal->master_pty < 0 ) {
            return terminal->master_pty;
        }

        terminal->lines = ( terminal_buffer_t* )kmalloc( sizeof( terminal_buffer_t ) * TERMINAL_MAX_LINES );

        if ( terminal->lines == NULL ) {
            return -ENOMEM;
        }

        memset( terminal->lines, 0, sizeof( terminal_buffer_t ) * TERMINAL_MAX_LINES );

        terminal->flags = TERMINAL_ACCEPTS_USER_INPUT;
        terminal->fg_color = COLOR_LIGHT_GRAY;
        terminal->bg_color = COLOR_BLACK;
        terminal->input_state = IS_NONE;
        terminal->line_count = 1;

        terminal->start_line = 0;

        terminal->cursor_row = 0;
        terminal->cursor_column = 0;

        terminals[ i ] = terminal;
    }

    read_thread = create_kernel_thread( "terminal read", PRIORITY_NORMAL, terminal_read_thread, NULL, 0 );

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

    error = mkdir( "/device/terminal", 0 );

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to create /device/terminal\n" );
        return error;
    }

    error = init_pty_filesystem();

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to initialize pty filesystem\n" );
        return error;
    }

    error = mount( "", "/device/terminal", "pty" );

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

MODULE_OPTIONAL_DEPENDENCIES( "ps2" );
