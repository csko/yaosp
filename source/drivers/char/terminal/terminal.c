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
#include <ioctl.h>
#include <input.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include "pty.h"
#include "terminal.h"
#include "kterm.h"

semaphore_id terminal_lock = -1;
thread_id read_thread;
terminal_t* active_terminal = NULL;
terminal_t* terminals[ MAX_TERMINAL_COUNT ];

static console_t* screen;

void terminal_do_full_update( terminal_t* terminal ) {
    int i;
    int j;
    char bg_color;
    char fg_color;
    terminal_buffer_t* buffer;
    terminal_buffer_item_t* data_item;

    /* Clear the screen */

    screen->ops->set_bg_color( screen, COLOR_BLACK );
    screen->ops->set_fg_color( screen, COLOR_LIGHT_GRAY );
    screen->ops->clear( screen );

    bg_color = COLOR_BLACK;
    fg_color = COLOR_LIGHT_GRAY;

    ASSERT( terminal->line_count > 0 );

    buffer = &terminal->lines[ terminal->start_line ];

    for ( i = 0; i < screen->height; i++, buffer++ ) {
        if ( ( buffer->data == NULL ) && ( i != screen->height - 1 ) ) {
            screen->ops->putchar( screen, '\n' );

            continue;
        }

        data_item = &buffer->data[ 0 ];

        for ( j = 0; j <= buffer->last_dirty; j++, data_item++ ) {
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

        if ( ( buffer->last_dirty != ( screen->width - 1 ) ) && ( i != ( screen->height - 1 ) ) ) {
            screen->ops->putchar( screen, '\n' );
        }
    }

    /* Move the cursor to the right position */

    /* TODO: do this only if it's a valid position on the real screen */
    screen->ops->gotoxy( screen, terminal->cursor_column, terminal->cursor_row - terminal->start_line );
}

int terminal_scroll( int offset ) {
    int tmp;
    int new_start_line;

    ASSERT( is_semaphore_locked( terminal_lock ) );

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

    return 0;
}

int terminal_switch_to( int index ) {
    if ( ( index < 0 ) || ( index >= MAX_TERMINAL_COUNT ) ) {
        return -EINVAL;
    }

    /* Make sure we're switching to another terminal */

    if ( terminals[ index ] == active_terminal ) {
        return 0;
    }

    active_terminal = terminals[ index ];

    terminal_do_full_update( active_terminal );

    screen->ops->set_bg_color( screen, active_terminal->bg_color );
    screen->ops->set_fg_color( screen, active_terminal->fg_color );

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
        tmp->last_dirty = -1;
    } else {
        if ( terminal->cursor_row == terminal->line_count - 1 ) {
            terminal->line_count++;
        }
    }

    if ( terminal->cursor_row < ( terminal->line_count - 1 ) ) {
        terminal->cursor_row++;
    }

    if ( terminal->start_line + screen->height == terminal->cursor_row ) {
        terminal->start_line++;
    }
}

void terminal_put_char( terminal_t* terminal, char c ) {
    terminal_buffer_t* buffer;
    terminal_buffer_item_t* data_item;

    /* Check if the current line has an allocated buffer */

    buffer = &terminal->lines[ terminal->cursor_row ];

    if ( buffer->data == NULL ) {
        int i;

        buffer->data = ( terminal_buffer_item_t* )kmalloc( sizeof( terminal_buffer_item_t ) * screen->width );

        if ( buffer->data == NULL ) {
            return;
        }

        data_item = &buffer->data[ 0 ];

        for ( i = 0; i < screen->width; i++, data_item++ ) {
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

    if ( terminal->cursor_column == screen->width ) {
        terminal_handle_new_line( terminal );
    }
}

static inline void terminal_move_cursor_to( terminal_t* terminal, int cursor_x, int cursor_y ) {
    terminal->cursor_row = terminal->start_line + cursor_y;
    terminal->cursor_column = cursor_x;

    if ( terminal == active_terminal ) {
        screen->ops->gotoxy( screen, cursor_x, cursor_y );
    }
}

static inline console_color_t ansi_to_console_color( bool is_bold, int color ) {
    switch ( color ) {
        case 0 : if ( is_bold ) { return COLOR_BLACK; } else { return COLOR_BLACK; }
        case 1 : if ( is_bold ) { return COLOR_LIGHT_RED; } else { return COLOR_RED; }
        case 2 : if ( is_bold ) { return COLOR_LIGHT_GREEN; } else { return COLOR_GREEN; }
        case 3 : return COLOR_BLACK; /* TODO: yellow? */
        case 4 : if ( is_bold ) { return COLOR_LIGHT_BLUE; } else { return COLOR_BLUE; }
        case 5 : if ( is_bold ) { return COLOR_LIGHT_MAGENTA; } else { return COLOR_MAGENTA; }
        case 6 : if ( is_bold ) { return COLOR_LIGHT_CYAN; } else { return COLOR_CYAN; }
        case 7 : if ( is_bold ) { return COLOR_WHITE; } else { return COLOR_LIGHT_GRAY; }
        default : return COLOR_BLACK;
    }
}

static inline void terminal_set_fg_color( terminal_t* terminal, int ansi_color ) {
    console_color_t color;

    ASSERT( ( ansi_color >= 30 ) && ( ansi_color <= 37 ) );

    color = ansi_to_console_color( terminal->is_bold, ansi_color - 30 );

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

    color = ansi_to_console_color( terminal->is_bold, ansi_color - 40 );

    if ( terminal->bg_color != color ) {
        terminal->bg_color = color;

        if ( terminal == active_terminal ) {
            screen->ops->set_bg_color( screen, color );
        }
    }
}

static void terminal_parse_data( terminal_t* terminal, char* data, size_t size ) {
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

                    case '\r' :
                        terminal_move_cursor_to( terminal, 0, terminal->cursor_row - terminal->start_line );
                        break;

                    case '\b' : {
                        int new_x;
                        int new_y;

                        if ( terminal->cursor_column == 0 ) {
                            if ( terminal->cursor_row == terminal->start_line ) {
                                new_x = terminal->cursor_column;
                                new_y = terminal->cursor_row;
                            } else {
                                new_x = screen->width - 1;
                                new_y = terminal->cursor_row - 1;
                            }
                        } else {
                            new_x = terminal->cursor_column - 1;
                            new_y = terminal->cursor_row;
                        }

                        if ( ( new_x != terminal->cursor_column ) ||
                             ( new_y != terminal->cursor_row ) ) {
                            terminal_move_cursor_to( terminal, new_x, new_y - terminal->start_line );
                        }

                        break;
                    }

                    default :
                        if ( ( c >= ' ' ) ||
                             ( c == '\n' ) ||
                             ( c == '\b' ) ) {
                            terminal_put_char( terminal, c );
                        }

                        break;
                }

                break;

            case IS_ESC :
                switch ( c ) {
                    case '[' :
                        terminal->input_state = IS_BRACKET;
                        break;

                    case '(' :
                        terminal->input_state = IS_OPEN_BRACKET;
                        break;

                    case ')' :
                        terminal->input_state = IS_CLOSE_BRACKET;
                        break;

                    case '=' :
                        /* TODO: Set alternate keypad mode */
                        terminal->input_state = IS_NONE;
                        break;

                    case '>' :
                        /* TODO: Set numeric keypad mode */
                        terminal->input_state = IS_NONE;
                        break;

                    default :
                        kprintf( "Terminal: Unknown character (%x) in IS_ESC state!\n", ( int )c & 0xFF );
                        terminal->input_state = IS_NONE;
                        break;
                }

                break;

            case IS_BRACKET :
                switch ( c ) {
                    case 's' :
                        terminal->saved_cursor_row = terminal->cursor_row;
                        terminal->saved_cursor_column = terminal->cursor_column;

                        terminal->input_state = IS_NONE;

                        break;

                    case 'u' :
                        terminal_move_cursor_to( terminal, terminal->saved_cursor_column, terminal->saved_cursor_row - terminal->start_line );

                        terminal->input_state = IS_NONE;

                        break;

                    case 'H' :
                    case 'f' : {
                        int new_x;
                        int new_y;

                        if ( terminal->input_param_count == 2 ) {
                            new_y = terminal->input_params[ 0 ] - 1;
                            new_x = terminal->input_params[ 1 ] - 1;
                        } else {
                            new_x = 0;
                            new_y = 0;
                        }

                        terminal_move_cursor_to( terminal, new_x, new_y );
                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'm' : {
                        int i;

                        if ( terminal->input_param_count == 0 ) {
                            terminal->is_bold = false;
                            terminal->fg_color = COLOR_LIGHT_GRAY;
                            terminal->bg_color = COLOR_BLACK;

                            if ( terminal == active_terminal ) {
                                screen->ops->set_bg_color( screen, terminal->bg_color );
                                screen->ops->set_fg_color( screen, terminal->fg_color );
                            }
                        } else {
                            for ( i = 0; i < terminal->input_param_count; i++ ) {
                                switch ( terminal->input_params[ i ] ) {
                                    case 0 :
                                        terminal->is_bold = false;
                                        terminal->fg_color = COLOR_LIGHT_GRAY;
                                        terminal->bg_color = COLOR_BLACK;

                                        if ( terminal == active_terminal ) {
                                            screen->ops->set_bg_color( screen, terminal->bg_color );
                                            screen->ops->set_fg_color( screen, terminal->fg_color );
                                        }

                                        break;

                                    case 1 :
                                        terminal->is_bold = true;
                                        break;

                                    case 2 :
                                        terminal->is_bold = false;
                                        break;

                                    case 7 : {
                                        console_color_t tmp;

                                        tmp = terminal->fg_color;
                                        terminal->fg_color = terminal->bg_color;
                                        terminal->bg_color = tmp;

                                        if ( terminal == active_terminal ) {
                                            screen->ops->set_bg_color( screen, terminal->bg_color );
                                            screen->ops->set_fg_color( screen, terminal->fg_color );
                                        }

                                        break;
                                    }

                                    case 30 ... 37 :
                                        terminal_set_fg_color( terminal, terminal->input_params[ i ] );
                                        break;

                                    case 40 ... 47 :
                                        terminal_set_bg_color( terminal, terminal->input_params[ i ] );
                                        break;

                                    default :
                                        kprintf( "Terminal: Invalid parameter (%d) for 'm' sequence!\n", terminal->input_params[ i ] );
                                        break;
                                }
                            }
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'J' :
                        if ( ( terminal->input_param_count == 0 ) ||
                             ( ( terminal->input_param_count == 1 ) && ( terminal->input_params[ 0 ] == 0 ) ) ) {
                            int i;
                            int j;
                            terminal_buffer_t* buffer;

                            /* Clear screen from cursor down */

                            buffer = &terminal->lines[ terminal->cursor_row ];

                            for ( i = terminal->cursor_row; i < terminal->start_line + screen->height; i++, buffer++ ) {
                                terminal_buffer_item_t* data_item;

                                if ( buffer->data != NULL ) {
                                    data_item = &buffer->data[ 0 ];

                                    for ( j = 0; j < screen->width; j++, data_item++ ) {
                                        data_item->character = ' ';
                                    }
                                }

                                buffer->last_dirty = -1;
                            }

                            /* Update the screen */

                            if ( terminal == active_terminal ) {
                                terminal_do_full_update( terminal );
                            }
                        } else if ( ( terminal->input_param_count == 1 ) && ( terminal->input_params[ 0 ] == 1 ) ) {
                            int i;
                            int j;
                            terminal_buffer_t* buffer;

                            /* Clear screen from cursor up */

                            buffer = &terminal->lines[ terminal->cursor_row ];

                            for ( i = terminal->cursor_row; i >= terminal->start_line; i--, buffer-- ) {
                                terminal_buffer_item_t* data_item;

                                if ( buffer->data != NULL ) {
                                    data_item = &buffer->data[ 0 ];

                                    for ( j = 0; j < screen->width; j++, data_item++ ) {
                                        data_item->character = ' ';
                                    }
                                }

                                buffer->last_dirty = -1;
                            }

                            /* Update the screen */

                            if ( terminal == active_terminal ) {
                                terminal_do_full_update( terminal );
                            }
                        } else if ( ( terminal->input_param_count == 1 ) && ( terminal->input_params[ 0 ] == 2 ) ) {
                            /* TODO: clear entire screen */
                        }

                        terminal->input_state = IS_NONE;

                        break;

                    case 'K' : {
                        int i;
                        terminal_buffer_t* buffer;

                        buffer = &terminal->lines[ terminal->cursor_row ];

                        if ( buffer->data != NULL ) {
                            for ( i = terminal->cursor_column; i < screen->width; i++ ) {
                                buffer->data[ i ].character = ' ';
                            }
                        }

                        buffer->last_dirty = terminal->cursor_column - 1;

                        if ( terminal == active_terminal ) {
                            /* TODO: Update only the touched line here, not the entire screen! */
                            terminal_do_full_update( terminal );
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'A' : {
                        int new_y;

                        if ( ( terminal->input_param_count == 0 ) || ( terminal->input_params[ 0 ] == 0 ) ) {
                            new_y = terminal->cursor_row - 1;
                        } else {
                            new_y = terminal->cursor_row - terminal->input_params[ 0 ];
                        }

                        if ( new_y >= terminal->start_line ) {
                            terminal_move_cursor_to( terminal, terminal->cursor_column, new_y - terminal->start_line );
                        } else {
                            terminal_move_cursor_to( terminal, terminal->cursor_column, 0 );
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'B' : {
                        int new_y;

                        if ( ( terminal->input_param_count == 0 ) || ( terminal->input_params[ 0 ] == 0 ) ) {
                            new_y = terminal->cursor_row + 1;
                        } else {
                            new_y = terminal->cursor_row + terminal->input_params[ 0 ];
                        }

                        if ( new_y < ( terminal->start_line + screen->height ) ) {
                            terminal_move_cursor_to( terminal, terminal->cursor_column, new_y - terminal->start_line );
                        } else {
                            terminal_move_cursor_to( terminal, terminal->cursor_column, screen->height - 1 );
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'C' : {
                        int new_x;

                        if ( ( terminal->input_param_count == 0 ) || ( terminal->input_params[ 0 ] == 0 ) ) {
                            new_x = terminal->cursor_column + 1;
                        } else {
                            new_x = terminal->cursor_column + terminal->input_params[ 0 ];
                        }

                        if ( new_x < screen->width ) {
                            terminal_move_cursor_to( terminal, new_x, terminal->cursor_row );
                        } else {
                            terminal_move_cursor_to( terminal, screen->width - 1, terminal->cursor_row );
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'D' : {
                        int new_x;

                        if ( ( terminal->input_param_count == 0 ) || ( terminal->input_params[ 0 ] == 0 ) ) {
                            new_x = terminal->cursor_column - 1;
                        } else {
                            new_x = terminal->cursor_column - terminal->input_params[ 0 ];
                        }

                        if ( new_x >= 0 ) {
                            terminal_move_cursor_to( terminal, new_x, terminal->cursor_row );
                        } else {
                            terminal_move_cursor_to( terminal, 0, terminal->cursor_row );
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'P' : {
                        terminal_buffer_t* buffer;

                        buffer = &terminal->lines[ terminal->cursor_row ];

                        if ( ( buffer != NULL ) &&
                             ( terminal->cursor_column < buffer->last_dirty ) ) {
                            memmove(
                                &buffer->data[ terminal->cursor_column ],
                                &buffer->data[ terminal->cursor_column + 1 ],
                                ( buffer->last_dirty - terminal->cursor_column ) * sizeof( terminal_buffer_item_t )
                            );

                            buffer->last_dirty--;

                            /* Update the screen */

                            if ( terminal == active_terminal ) {
                                terminal_do_full_update( terminal );
                            }
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case '@' : {
                        terminal_buffer_t* buffer;

                        buffer = &terminal->lines[ terminal->cursor_row ];

                        /* TODO: We should insert the last character to the next line if we insert to a full line? */

                        if ( ( buffer != NULL ) &&
                             ( buffer->last_dirty < ( screen->width - 1 ) ) ) {
                            terminal_buffer_item_t* data_item;

                            ASSERT( buffer->last_dirty >= terminal->cursor_column );

                            memmove(
                                &buffer->data[ terminal->cursor_column + 1 ],
                                &buffer->data[ terminal->cursor_column ],
                                ( buffer->last_dirty - terminal->cursor_column + 1 ) * sizeof( terminal_buffer_item_t )
                            );

                            data_item = &buffer->data[ terminal->cursor_column ];

                            data_item->character = ' ';
                            data_item->fg_color = COLOR_LIGHT_GRAY;
                            data_item->bg_color = COLOR_BLACK;

                            buffer->last_dirty++;

                            /* Update the screen */

                            if ( terminal == active_terminal ) {
                                terminal_do_full_update( terminal );
                            }
                        }

                        terminal->input_state = IS_NONE;

                        break;
                    }

                    case 'r' :
                        /* TODO: what's this? */
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

                    case '?' :
                        terminal->input_state = IS_QUESTION;
                        break;

                    default :
                        kprintf( "Terminal: Unknown character (%x) in IS_BRACKET state!\n", ( int )c & 0xFF );
                        terminal->input_state = IS_NONE;
                        break;
                }

                break;

            case IS_OPEN_BRACKET :
                switch ( c ) {
                    case 'A' :
                        /* TODO: Set United Kingdom G0 character set */
                        terminal->input_state = IS_NONE;
                        break;

                    case 'B' :
                        /* TODO: Set United States G0 character set */
                        terminal->input_state = IS_NONE;
                        break;

                    case '0' :
                        /* TODO: Set G0 special chars. & line set */
                        terminal->input_state = IS_NONE;
                        break;

                    default :
                        kprintf( "Terminal: Unknown character (%x) in IS_OPEN_BRACKET state!\n", ( int )c & 0xFF );
                        terminal->input_state = IS_NONE;
                        break;
                }

                break;

            case IS_CLOSE_BRACKET :
                switch ( c ) {
                    case 'A' :
                        /* TODO: Set United Kingdom G1 character set */
                        terminal->input_state = IS_NONE;
                        break;

                    case 'B' :
                        /* TODO: Set United States G1 character set */
                        terminal->input_state = IS_NONE;
                        break;

                    case '0' :
                        /* TODO: Set G1 special chars. & line set */
                        terminal->input_state = IS_NONE;
                        break;

                    default :
                        kprintf( "Terminal: Unknown character (%x) in IS_CLOSE_BRACKET state!\n", ( int )c & 0xFF );
                        terminal->input_state = IS_NONE;
                        break;
                }

                break;

            case IS_QUESTION :
                switch ( c ) {
                    case '0' ... '9' :
                        if ( terminal->input_param_count == 0 ) {
                            terminal->input_param_count++;
                            terminal->input_params[ 0 ] = ( c - '0' );
                        } else {
                            terminal->input_params[ terminal->input_param_count - 1 ] *= 10;
                            terminal->input_params[ terminal->input_param_count - 1 ] += ( c - '0' );
                        }

                        break;

                    case 'h' :
                        /* TODO */
                        terminal->input_state = IS_NONE;
                        break;

                    case 'l' :
                        /* TODO */
                        terminal->input_state = IS_NONE;
                        break;

                    default :
                        kprintf( "Terminal: Unknown character (%x) in IS_QUESTION state!\n", ( int )c & 0xFF );
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

        if ( __unlikely( count < 0 ) ) {
            kprintf( "Terminal: Select error: %d\n", count );
            continue;
        }

        for ( i = 0; i < MAX_TERMINAL_COUNT; i++ ) {
            if ( FD_ISSET( terminals[ i ]->master_pty, &read_set ) ) {
                size = pread( terminals[ i ]->master_pty, buffer, sizeof( buffer ), 0 );

                if ( __likely( size > 0 ) ) {
                    LOCK( terminal_lock );

                    terminal_parse_data( terminals[ i ], buffer, size );

                    UNLOCK( terminal_lock );
                }
            }
        }
    }

    return 0;
}

int init_terminals( void ) {
    int i;
    int j;
    char path[ 128 ];
    terminal_t* terminal;
    terminal_buffer_t* buffer;

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

        buffer = &terminal->lines[ 0 ];

        for ( j = 0; j < TERMINAL_MAX_LINES; j++, buffer++ ) {
            buffer->data = NULL;
            buffer->last_dirty = -1;
        }

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
    int error;

    terminal_lock = create_semaphore( "terminal lock", SEMAPHORE_BINARY, 0, 1 );

    if ( terminal_lock < 0 ) {
        return terminal_lock;
    }

    error = mkdir( "/device/terminal", 0777 );

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to create /device/terminal\n" );
        return error;
    }

    error = init_pty_filesystem();

    if ( error < 0 ) {
        kprintf( "Terminal: Failed to initialize pty filesystem\n" );
        return error;
    }

    error = mount( "", "/device/terminal", "pty", MOUNT_NONE );

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

    error = init_terminal_input();

    if ( error < 0 ) {
        return error;
    }

    error = init_terminal_ctrl_device();

    if ( error < 0 ) {
        return error;
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}

MODULE_DEPENDENCIES( "input" );
