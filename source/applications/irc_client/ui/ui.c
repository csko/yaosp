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

#include <unistd.h>
#include <ncurses.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "view.h"
#include "../core/event.h"
#include "../core/eventmanager.h"

#define MAX_INPUT_SIZE 255

static WINDOW* screen;
static WINDOW* win_title;
static WINDOW* win_main;
static WINDOW* win_status;
static WINDOW* win_input;

static int screen_w = 0;
static int screen_h = 0;
static event_t stdin_event;

static int cur_input_size = 0;
static char input_line[ MAX_INPUT_SIZE + 1 ];

view_t* active_view;

static int get_terminal_size( int* width, int* height ) {
    int error;
    struct winsize size;

    error = ioctl( STDIN_FILENO, TIOCGWINSZ, &size );

    if ( error >= 0 ) {
        *width = size.ws_col;
        *height = size.ws_row;
    }

    return error;
}

int ui_handle_command( const char* command, const char* params ) {
    if ( strcmp( command, "/quit" ) == 0 ) {
        event_manager_quit();
    }

    return 0;
}

static int ui_stdin_event( event_t* event ) {
    int c;

    while ( ( c = getch() ) != EOF ) {
        switch ( c ) {
            case '\n' : {
                char* params;

                params = strchr( input_line, ' ' );

                if ( params != NULL ) {
                    *params++ = 0;
                }

                active_view->operations->handle_command( input_line, params );

                cur_input_size = 0;
                input_line[ 0 ] = 0;

                break;
            }

            case '\b' :
                if ( cur_input_size > 0 ) {
                    input_line[ --cur_input_size ] = 0;
                }

                break;

            default :
                if ( cur_input_size < MAX_INPUT_SIZE ) {
                    input_line[ cur_input_size++ ] = c;
                    input_line[ cur_input_size ] = 0;
                }

                break;
        }

        wclear( win_input );
        mvwprintw( win_input, 0, 0, "%s", input_line );
        wrefresh( win_input );
    }

    return 0;
}

void ui_draw_view( view_t* view ) {
    int i;
    int start_line;
    char* line;

    start_line = array_get_size( &view->lines ) - ( screen_h - 2 );

    if ( start_line < 0 ) {
        start_line = 0;
    }

    wclear( win_main );

    for ( i = start_line; i < array_get_size( &view->lines ); i++ ) {
        line = ( char* )array_get_item( &view->lines, i );

        mvwprintw( win_main, i - start_line, 0, "%s", line );
    }

    wrefresh( win_main );
}

int ui_activate_view( view_t* view ) {
    int x;
    size_t length;
    const char* title;

    active_view = view;

    /* Update the title bar */

    wclear( win_title );

    title = view->operations->get_title();
    length = strlen( title );

    x = ( screen_w - length ) / 2;

    if ( x < 0 ) {
        x = 0;
    }

    mvwprintw( win_title, 0, x, "%s", title );
    wrefresh( win_title );

    /* Update the main view */

    ui_draw_view( view );

    return 0;
}

int init_ui( void ) {
    /* Initialize ncurses */

    screen = initscr();
    noecho();
    cbreak();
    nodelay( screen, TRUE );
    refresh();

    /* Get the terminal size */

    get_terminal_size( &screen_w, &screen_h );

    /* Create the windows */

    win_title = newwin( 1, screen_w, 0, 0 );
    win_main = newwin( screen_h - 3, screen_w, 1, 0 );
    win_status = newwin( 1, screen_w, screen_h - 2, 0 );
    win_input = newwin( 1, screen_w, screen_h - 1, 0 );

    refresh();

    /* Initialize our views */

    init_server_view();
    ui_activate_view( &server_view );

    /* Register an event for stdin */

    stdin_event.fd = STDIN_FILENO;
    stdin_event.events[ EVENT_READ ].interested = 1;
    stdin_event.events[ EVENT_READ ].callback = ui_stdin_event;
    stdin_event.events[ EVENT_WRITE ].interested = 0;
    stdin_event.events[ EVENT_EXCEPT ].interested = 0;

    event_manager_add_event( &stdin_event );

    return 0;
}

int destroy_ui( void ) {
    return 0;
}
