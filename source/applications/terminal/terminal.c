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

extern window_t* window;
extern widget_t* scrollpanel;
extern widget_t* terminal_widget;

static int terminal_update_widget( void* data ) {
    /* The preferred size of the widget may changed. Signal the
       event listeners ... */

    widget_signal_event_handler(
        terminal_widget,
        terminal_widget->event_ids[ E_PREF_SIZE_CHANGED ]
    );

    /* Move the scrollpanel to the bottom */

    int v_size = scroll_panel_get_v_size( scrollpanel );

    scroll_panel_set_v_offset(
        scrollpanel,
        v_size
    );

    return 0;
}

static int terminal_ensure_line( terminal_t* terminal, int line ) {
    int i;
    char* buffer;
    terminal_line_t* term_line;
    terminal_color_item_t* color_item;

    if ( ( line < 0 ) ||
         ( line >= terminal->max_lines ) ) {
        return -EINVAL;
    }

    term_line = &terminal->lines[ line ];

    if ( term_line->max_size >= terminal->width ) {
        return 0;
    }

    term_line->buffer = ( char* )realloc( term_line->buffer, terminal->width );

    if ( term_line->buffer == NULL ) {
        return -ENOMEM;
    }

    term_line->buffer_color = ( terminal_color_item_t* )realloc(
        term_line->buffer_color,
        sizeof( terminal_color_item_t ) * terminal->width
    );

    if ( term_line->buffer_color == NULL ) {
        return -ENOMEM;
    }

    for ( i = term_line->max_size,
          buffer = term_line->buffer + term_line->max_size,
          color_item = &term_line->buffer_color[ term_line->max_size ];
          i < terminal->width;
          i++, buffer++, color_item++ ) {
        *buffer = ' ';

        color_item->bg_color = T_COLOR_BLACK;
        color_item->fg_color = T_COLOR_WHITE;
    }

    term_line->max_size = terminal->width;

    return 0;
}

static void terminal_update_mode( terminal_t* terminal ) {
    int i;

    for ( i = 0; i < terminal->parameter_count; i++ ) {
        switch ( terminal->parameters[ i ] ) {
            case 0 :
                terminal->bg_color = T_COLOR_BLACK;
                terminal->fg_color = T_COLOR_WHITE;
                break;

            case 1 :
                break;

            case 7 : {
                terminal_color_t tmp;

                tmp = terminal->bg_color;
                terminal->bg_color = terminal->fg_color;
                terminal->fg_color = tmp;

                break;
            }

            case 30 ... 37 :
                terminal->fg_color = terminal->parameters[ i ] - 30;
                break;

            case 39 :
                terminal->fg_color = T_COLOR_WHITE;
                break;

            case 40 ... 47 :
                terminal->bg_color = terminal->parameters[ i ] - 40;
                break;

            case 49 :
                terminal->bg_color = T_COLOR_BLACK;
                break;

            default :
                dbprintf( "%s(): Unknown mode parameter: %d\n", __FUNCTION__, terminal->parameters[ i ] );
                break;
        }
    }
}

static void terminal_data_state_none( terminal_t* terminal, uint8_t data ) {
    int error;

    switch ( data ) {
        case 27 :
            terminal->state = STATE_ESCAPE;
            terminal->first_number = 1;
            terminal->parameter_count = 0;
            break;

        case '\r' :
            terminal->cursor_x = 0;
            break;

        case '\n' :
            terminal->cursor_y++;
            terminal->cursor_x = 0;
            break;

        case '\b' :
            if ( terminal->cursor_x == 0 ) {
                if ( terminal->cursor_y > 0 ) {
                    terminal->cursor_y--;
                    terminal->cursor_x = terminal->width - 1;
                }
            } else {
                terminal->cursor_x--;
            }

            break;

        case '\t' :
            /* Just skip tab for now ... */
            break;

        default : {
            terminal_line_t* term_line;

            if ( data < 32 ) {
                break;
            }

            error = terminal_ensure_line( terminal, terminal->cursor_y );

            if ( error < 0 ) {
                return;
            }

            term_line = &terminal->lines[ terminal->cursor_y ];

            term_line->buffer[ terminal->cursor_x ] = data;
            term_line->buffer_color[ terminal->cursor_x ].bg_color = terminal->bg_color;
            term_line->buffer_color[ terminal->cursor_x ].fg_color = terminal->fg_color;

            terminal->cursor_x++;

            term_line->size = terminal->cursor_x;

            if ( terminal->cursor_x == terminal->width ) {
                terminal->cursor_x = 0;
                terminal->cursor_y++;
            }

            terminal->last_line = MAX( terminal->last_line, terminal->cursor_y );

            break;
        }
    }
}

static void terminal_data_state_escape( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case '(' :
            terminal->state = STATE_BRACKET;
            break;

        case '[' :
            terminal->state = STATE_SQUARE_BRACKET;
            break;

        default :
            dbprintf( "%s(): Unhandled data: %d\n", __FUNCTION__, data );
            terminal->state = STATE_NONE;
            break;
    }
}

static void terminal_data_state_bracket( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case 'B' :
            /* TODO */
            terminal->state = STATE_NONE;
            break;

        default :
            dbprintf( "%s(): Unhandled data: %d\n", __FUNCTION__, data );
            terminal->state = STATE_NONE;
            break;
    }
}

static void terminal_data_state_square_bracket( terminal_t* terminal, uint8_t data ) {
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
                int i;
                terminal_line_t* line;

                /* TODO: not fully OK! */

                line = &terminal->lines[ terminal->cursor_y ];

                for ( i = terminal->cursor_y; i < terminal->height; i++, line++ ) {
                    line->size = 0;
                }
            } else {
                int i;
                terminal_line_t* line;

                switch ( terminal->parameters[ 0 ] ) {
                    case 1 :
                        line = &terminal->lines[ terminal->cursor_y ];

                        for ( i = terminal->cursor_y; i >= 0; i--, line-- ) {
                            line->size = 0;
                        }

                        break;

                    case 2 :
                        line = &terminal->lines[ 0 ];

                        for ( i = 0; i < terminal->height; i++, line++ ) {
                            line->size = 0;
                        }

                        terminal->cursor_x = 0;
                        terminal->cursor_y = 0;

                        break;
                }
            }

            terminal->state = STATE_NONE;

            break;

        case 'K' :
            assert( ( terminal->parameter_count == 0 ) ||
                    ( terminal->parameter_count == 1 ) );

            if ( terminal->parameter_count == 0 ) {
                int i;
                char* buffer;

                buffer = terminal->lines[ terminal->cursor_y ].buffer + terminal->cursor_x;

                for ( i = terminal->cursor_x; i < terminal->width; i++, buffer++ ) {
                    *buffer = ' ';
                }
            } else {
                int i;
                char* buffer;

                switch ( terminal->parameters[ 0 ] ) {
                    case 1 :
                        buffer = terminal->lines[ terminal->cursor_y ].buffer + terminal->cursor_x;

                        for ( i = terminal->cursor_x; i >= 0; i--, buffer-- ) {
                            *buffer = ' ';
                        }

                        break;

                    case 2 :
                        buffer = terminal->lines[ terminal->cursor_y ].buffer;

                        for ( i = 0; i < terminal->width; i++, buffer++ ) {
                            *buffer = ' ';
                        }

                        break;

                    default :
                        dbprintf( "%s(): Unhandled parameter at K: %d\n", terminal->parameters[ 0 ] );
                        break;
                }
            }

            terminal->state = STATE_NONE;

            break;

        case 'f' :
        case 'H' :
            assert( ( terminal->parameter_count == 0 ) ||
                    ( terminal->parameter_count == 2 ) );

            /* TODO: y setting is not fully correct! */

            switch ( terminal->parameter_count ) {
                case 0 :
                    terminal->cursor_y = 0;
                    terminal->cursor_x = 0;
                    break;

                case 2 :
                    terminal->cursor_y = terminal->parameters[ 0 ] - 1;
                    terminal->cursor_x = terminal->parameters[ 1 ] - 1;
                    break;
            }

            terminal->state = STATE_NONE;

            break;

        case 'd' :
            assert( terminal->parameter_count == 1 );

            /* TODO: y setting is not fully correct! */

            terminal->cursor_y = terminal->parameters[ 0 ] = 1;
            terminal->state = STATE_NONE;

            break;

        case 'G' :
            assert( terminal->parameter_count == 1 );

            terminal->cursor_x = terminal->parameters[ 0 ] - 1;
            terminal->state = STATE_NONE;

            break;

        case 'r' :
            /* TODO: scrolling */
            terminal->state = STATE_NONE;
            break;

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

static void terminal_data_state_question( terminal_t* terminal, uint8_t data ) {
    switch ( data ) {
        case '0' ... '9' :
            break;

        case 'h' :
            terminal->state = STATE_NONE;
            break;

        case 'l' :
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
    int new_last_line;
    int current_last_line;

    pthread_mutex_lock( &terminal->lock );

    current_last_line = terminal->last_line;

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

    new_last_line = terminal->last_line;

    pthread_mutex_unlock( &terminal->lock );

    window_insert_callback( window, terminal_update_widget, NULL );

    return 0;
}

terminal_t* create_terminal( int width, int height ) {
    terminal_t* terminal;

    terminal = ( terminal_t* )malloc( sizeof( terminal_t ) );

    if ( terminal == NULL ) {
        goto error1;
    }

    terminal->max_lines = 500;

    terminal->lines = ( terminal_line_t* )malloc( sizeof( terminal_line_t ) * terminal->max_lines );

    if ( terminal->lines == NULL ) {
        goto error2;
    }

    memset( terminal->lines, 0, sizeof( terminal_line_t ) * terminal->max_lines );

    if ( pthread_mutex_init( &terminal->lock, NULL ) < 0 ) {
        goto error3;
    }

    terminal->width = width;
    terminal->height = height;
    terminal->cursor_x = 0;
    terminal->cursor_y = 0;
    terminal->bg_color = T_COLOR_BLACK;
    terminal->fg_color = T_COLOR_WHITE;
    terminal->last_line = 0;

    return terminal;

 error3:
    free( terminal->lines );

 error2:
    free( terminal );

 error1:
    return NULL;
}

int destroy_terminal( terminal_t* terminal ) {
    /* TODO */

    return 0;
}

