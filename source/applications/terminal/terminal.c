/* Terminal application
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
#include <assert.h>
#include <yaosp/debug.h>

#include <ygui/window.h>
#include <ygui/scrollpanel.h>

#include "terminal.h"
#include "term_widget.h"

void terminal_update_mode( terminal_t* terminal ) {
    int i;

    if ( terminal->parameter_count == 0 ) {
        terminal_buffer_set_bg( &terminal->buffer, T_COLOR_BLACK );
        terminal_buffer_set_fg( &terminal->buffer, T_COLOR_WHITE );

        return;
    }

    for ( i = 0; i < terminal->parameter_count; i++ ) {
        switch ( terminal->parameters[ i ] ) {
            case 0 :
                terminal_buffer_set_bg( &terminal->buffer, T_COLOR_BLACK );
                terminal_buffer_set_fg( &terminal->buffer, T_COLOR_WHITE );
                break;

            case 1 :
                /* bold */
                break;

            case 2 :
                /* normal */
                break;

            case 7 :
                terminal_buffer_swap_bgfg( &terminal->buffer );
                break;

            case 30 ... 37 :
                terminal_buffer_set_fg( &terminal->buffer, terminal->parameters[ i ] - 30 );
                break;

            case 39 :
                terminal_buffer_set_fg( &terminal->buffer, T_COLOR_WHITE );
                break;

            case 40 ... 47 :
                terminal_buffer_set_bg( &terminal->buffer, terminal->parameters[ i ] - 40 );
                break;

            case 49 :
                terminal_buffer_set_bg( &terminal->buffer, T_COLOR_BLACK );
                break;

            default :
                dbprintf( "%s(): Unknown mode parameter: %d\n", __FUNCTION__, terminal->parameters[ i ] );
                break;
        }
    }
}

void terminal_data_state_none( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case 27 :
            terminal->state = STATE_ESCAPE;
            terminal->first_number = 1;
            terminal->parameter_count = 0;
            break;

        case '\r' :
            terminal_buffer_insert_cr( &terminal->buffer );
            break;

        case '\n' :
            terminal_buffer_insert_lf( &terminal->buffer );
            break;

        case '\b' :
            terminal_buffer_insert_backspace( &terminal->buffer );
            break;

        case '\t' :
            /* Just skip tab for now ... */
            break;

        default :
            if ( data < 32 ) {
                break;
            }

            terminal_buffer_insert_char(
                &terminal->buffer,
                data
            );

            break;
    }
}

void terminal_data_state_escape( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case '(' :
            terminal->state = STATE_BRACKET;
            break;

        case '[' :
            terminal->state = STATE_SQUARE_BRACKET;
            break;

        case 'D' :
            terminal_buffer_move_cursor(
                &terminal->buffer,
                0,
                1
            );

            terminal->state = STATE_NONE;

            break;

        case 'M' :
            terminal_buffer_scroll_by(
                &terminal->buffer,
                -1
            );

#if 0
            terminal_buffer_move_cursor(
                &terminal->buffer,
                0,
                -1
            );
#endif

            terminal->state = STATE_NONE;

            break;

        case '7' :
            terminal_buffer_save_cursor( &terminal->buffer );
            terminal_buffer_save_attr( &terminal->buffer );
            terminal->state = STATE_NONE;
            break;

        case '8' :
            terminal_buffer_restore_cursor( &terminal->buffer );
            terminal_buffer_restore_attr( &terminal->buffer );
            terminal->state = STATE_NONE;
            break;

        case '>' :
        case '=' :
            /* ??? */
            terminal->state = STATE_NONE;
            break;

        default :
            dbprintf( "%s(): Unhandled data: %d\n", __FUNCTION__, data );
            terminal->state = STATE_NONE;
            break;
    }
}

void terminal_data_state_bracket( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case 'B' :
            /* North American ASCII set */
            terminal->state = STATE_NONE;
            break;

        default :
            dbprintf( "%s(): Unhandled data: %d\n", __FUNCTION__, data );
            terminal->state = STATE_NONE;
            break;
    }
}

void terminal_data_state_square_bracket( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case '0' ... '9' :
            if ( terminal->first_number ) {
                terminal->first_number = 0;
                terminal->parameters[ terminal->parameter_count++ ] = ( data - '0' );
            } else {
                terminal->parameters[ terminal->parameter_count - 1 ] =
                    terminal->parameters[ terminal->parameter_count - 1 ] * 10 + ( data - '0' );
            }

            break;

        case ';' :
            terminal->first_number = 1;
            break;

        case 'm' :
            terminal_update_mode( terminal );
            terminal->state = STATE_NONE;
            break;

        case 'J' :
            assert( ( terminal->parameter_count == 0 ) ||
                    ( terminal->parameter_count == 1 ) );

            if ( terminal->parameter_count == 0 ) {
                terminal_buffer_erase_below( &terminal->buffer );
            } else {
                switch ( terminal->parameters[ 0 ] ) {
                    case 1 :
                        terminal_buffer_erase_above( &terminal->buffer );
                        break;

                    case 2 :
                        terminal_buffer_erase_above( &terminal->buffer );
                        terminal_buffer_erase_below( &terminal->buffer );

                        terminal_buffer_move_cursor_to( &terminal->buffer, 0, 0 );

                        break;
                }
            }

            terminal->state = STATE_NONE;

            break;

        case 'K' :
            assert( ( terminal->parameter_count == 0 ) ||
                    ( terminal->parameter_count == 1 ) );

            if ( terminal->parameter_count == 0 ) {
                terminal_buffer_erase_after( &terminal->buffer );
            } else {
                switch ( terminal->parameters[ 0 ] ) {
                    case 1 :
                        terminal_buffer_erase_before( &terminal->buffer );
                        break;

                    case 2 :
                        terminal_buffer_erase_before( &terminal->buffer );
                        terminal_buffer_erase_after( &terminal->buffer );
                        break;
                }
            }

            terminal->state = STATE_NONE;

            break;

        case 'f' :
        case 'H' :
            assert( ( terminal->parameter_count == 0 ) ||
                    ( terminal->parameter_count == 2 ) );

            switch ( terminal->parameter_count ) {
                case 0 :
                    terminal_buffer_move_cursor_to( &terminal->buffer, 0, 0 );
                    break;

                case 2 :
                    terminal_buffer_move_cursor_to(
                        &terminal->buffer,
                        terminal->parameters[ 1 ] - 1,
                        terminal->parameters[ 0 ] - 1
                    );

                    break;
            }

            terminal->state = STATE_NONE;

            break;

        case 'd' :
            assert( terminal->parameter_count == 1 );

            terminal_buffer_move_cursor_to(
                &terminal->buffer,
                -1,
                terminal->parameters[ 0 ] - 1
            );

            terminal->state = STATE_NONE;

            break;

        case 'G' :
            assert( terminal->parameter_count == 1 );

            terminal_buffer_move_cursor_to(
                &terminal->buffer,
                terminal->parameters[ 0 ] - 1,
                -1
            );

            terminal->state = STATE_NONE;

            break;

        case 'A' : {
            int dy = 1;

            if ( terminal->parameter_count == 1 ) {
                dy = terminal->parameters[ 0 ];
            }

            terminal_buffer_move_cursor(
                &terminal->buffer,
                0,
                -dy
            );

            terminal->state = STATE_NONE;

            break;
        }

        case 'B' : {
            int dy = 1;

            if ( terminal->parameter_count == 1 ) {
                dy = terminal->parameters[ 0 ];
            }

            terminal_buffer_move_cursor(
                &terminal->buffer,
                0,
                dy
            );

            terminal->state = STATE_NONE;

            break;
        }

        case 'C' : {
            int dx = 1;

            if ( terminal->parameter_count == 1 ) {
                dx = terminal->parameters[ 0 ];
            }

            terminal_buffer_move_cursor(
                &terminal->buffer,
                dx,
                0
            );

            terminal->state = STATE_NONE;

            break;
        }

        case 'D' : {
            int dx = 1;

            if ( terminal->parameter_count == 1 ) {
                dx = terminal->parameters[ 0 ];
            }

            terminal_buffer_move_cursor(
                &terminal->buffer,
                -dx,
                0
            );

            terminal->state = STATE_NONE;

            break;
        }

        case 'r' :
            assert( terminal->parameter_count == 2 );

            terminal_buffer_set_scroll_region(
                &terminal->buffer,
                terminal->parameters[ 0 ] - 1,
                terminal->parameters[ 1 ] - 1
            );

            terminal->state = STATE_NONE;

            break;

        case 'S' :
            assert( terminal->parameter_count == 1 );

            terminal_buffer_scroll_by(
                &terminal->buffer,
                terminal->parameters[ 0 ]
            );

            terminal->state = STATE_NONE;

            break;

        case 'X' : {
            int count;

            if ( terminal->parameter_count == 0 ) {
                count = 1;
            } else {
                count = terminal->parameters[ 0 ];
            }

            terminal_buffer_erase( &terminal->buffer, count );

            terminal->state = STATE_NONE;

            break;
        }

        case 'T' :
            assert( terminal->parameter_count == 1 );

            terminal_buffer_scroll_by(
                &terminal->buffer,
                -terminal->parameters[ 0 ]
            );

            terminal->state = STATE_NONE;

            break;

        case '@' : {
            int count = 1;

            if ( terminal->parameter_count == 1 ) {
                count = terminal->parameters[ 0 ];
            }

            terminal_buffer_insert_space(
                &terminal->buffer,
                count
            );

            terminal->state = STATE_NONE;

            break;
        }

        case 'P' : {
            int count = 1;

            if ( terminal->parameter_count == 1 ) {
                count = terminal->parameters[ 0 ];
            }

            terminal_buffer_delete(
                &terminal->buffer,
                count
            );

            terminal->state = STATE_NONE;

            break;
        }

        case 'l' :
            /* TODO */
            terminal->state = STATE_NONE;
            break;

        case '?' :
            terminal->state = STATE_QUESTION;
            break;

        default :
            dbprintf( "%s(): Unhandled data: %d\n", __FUNCTION__, data );
            terminal->state = STATE_NONE;
            break;
    }
}

void terminal_data_state_question( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case '0' ... '9' :
            if ( terminal->first_number ) {
                terminal->first_number = 0;
                terminal->parameters[ terminal->parameter_count++ ] = ( data - '0' );
            } else {
                terminal->parameters[ terminal->parameter_count - 1 ] =
                    terminal->parameters[ terminal->parameter_count - 1 ] * 10 + ( data - '0' );
            }

            break;

        case 'h' :
            assert( terminal->parameter_count == 1 );

            switch ( terminal->parameters[ 0 ] ) {
                case 1 :
                    /* TODO: set alternate cursor keys */
                    break;

                default :
                    //dbprintf( "%s(): h with param: %d\n", __FUNCTION__, terminal->parameters[ 0 ] );
                    break;
            }

            terminal->state = STATE_NONE;

            break;

        case 'l' :
            assert( terminal->parameter_count == 1 );

            switch ( terminal->parameters[ 0 ] ) {
                case 1 :
                    /* TODO: unset alternate cursor keys */
                    break;

                default :
                    //dbprintf( "%s(): l with param: %d\n", __FUNCTION__, terminal->parameters[ 0 ] );
                    break;
            }

            terminal->state = STATE_NONE;

            break;

        default :
            dbprintf( "%s(): Unhandled data: %d\n", __FUNCTION__, data );
            terminal->state = STATE_NONE;
            break;
    }
}

int terminal_handle_data( terminal_t* terminal, uint8_t* data, int size ) {
    int i;

    pthread_mutex_lock( &terminal->lock );

    for ( i = 0; i < size; i++, data++ ) {
        switch ( terminal->state ) {
            case STATE_NONE :
                terminal_data_state_none( terminal, *data );
                break;

            case STATE_ESCAPE :
                terminal_data_state_escape( terminal, *data );
                break;

            case STATE_BRACKET :
                terminal_data_state_bracket( terminal, *data );
                break;

            case STATE_SQUARE_BRACKET :
                terminal_data_state_square_bracket( terminal, *data );
                break;

            case STATE_QUESTION :
                terminal_data_state_question( terminal, *data );
                break;
        }
    }

    pthread_mutex_unlock( &terminal->lock );

    return 0;
}

terminal_t* create_terminal( int width, int height ) {
    int error;
    terminal_t* terminal;

    terminal = ( terminal_t* )malloc( sizeof( terminal_t ) );

    if ( terminal == NULL ) {
        goto error1;
    }

    error = terminal_buffer_init( &terminal->buffer, width, height );

    if ( error < 0 ) {
        goto error2;
    }

    if ( pthread_mutex_init( &terminal->lock, NULL ) < 0 ) {
        goto error3;
    }

    terminal->state = STATE_NONE;

    return terminal;

 error3:
    /* TODO */

 error2:
    free( terminal );

 error1:
    return NULL;
}

int destroy_terminal( terminal_t* terminal ) {
    /* TODO */

    return 0;
}

