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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <yaosp/debug.h>

#include <ygui/application.h>
#include <ygui/window.h>
#include <ygui/panel.h>
#include <ygui/textfield.h>
#include <ygui/button.h>
#include <ygui/scrollpanel.h>
#include <ygui/layout/borderlayout.h>

#include "term_widget.h"

window_t* window;
widget_t* scrollpanel;
widget_t* terminal_widget;

int master_pty;
pid_t shell_pid;
terminal_t* terminal;

pthread_t pty_read_thread;

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

static void* pty_read_thread_entry( void* arg ) {
    int size;
    uint8_t buffer[ 512 ];

    while ( 1 ) {
        size = read( master_pty, buffer, sizeof( buffer ) );

        if ( size < 0 ) {
            dbprintf( "Failed to read from the master PTY!\n" );

            break;
        }

        if ( size == 0 ) {
            continue;
        }

        /* Add the new data to the terminal engine */

        terminal_handle_data( terminal, buffer, size );

        /* Invalidate the terminal widget to repaint it */

        window_insert_callback( window, terminal_update_widget, NULL );
    }

    return NULL;
}

static int initialize_terminal( void ) {
    terminal = create_terminal( 80, 25 );

    if ( terminal == NULL ) {
        return -ENOMEM;
    }

    return 0;
}

static int initialize_pty( void ) {
    int i;
    char path[ 64 ];

    /* Create the pty for the terminal<->shell communication */

    i = 0;

    while ( 1 ) {
        struct stat st;

        snprintf( path, sizeof( path ), "/device/terminal/pty%d", i );

        if ( stat( path, &st ) != 0 ) {
            break;
        }

        i++;
    }

    master_pty = open( path, O_RDWR | O_CREAT );

    if ( master_pty < 0 ) {
        dbprintf( "Failed to open the master PTY!\n" );

        return EXIT_FAILURE;
    }

    shell_pid = fork();

    if ( shell_pid == 0 ) {
        int error;
        int slave_tty;
        struct winsize win_size;

        snprintf( path, sizeof( path ), "/device/terminal/tty%d", i );

        slave_tty = open( path, O_RDWR );

        if ( slave_tty < 0 ) {
            dbprintf( "Failed to open the slave TTY!\n" );

            _exit( 1 );
        }

        dup2( slave_tty, 0 );
        dup2( slave_tty, 1 );
        dup2( slave_tty, 2 );

        win_size.ws_col = 80;
        win_size.ws_row = 24;

        ioctl( slave_tty, TIOCSWINSZ, &win_size );

        char* argv[] = { "bash", NULL };

        error = execv( "/application/bash", argv );

        if ( error < 0 ) {
            dbprintf( "Failed to start the shell!\n" );
        }

        _exit( 1 );
    } else if ( shell_pid < 0 ) {
        return shell_pid;
    }

    pthread_attr_t attrib;

    pthread_attr_init( &attrib );
    pthread_attr_setname( &attrib, "pty_reader" );

    pthread_create(
        &pty_read_thread,
        &attrib,
        pty_read_thread_entry,
        NULL
    );

    pthread_attr_destroy( &attrib );

    return 0;
}

static int initialize_gui( void ) {
    /* Create the GUI */

    point_t tmp;
    point_t point = { .x = 50, 50 };
    point_t size;

    scrollpanel = create_scroll_panel( SCROLLBAR_ALWAYS, SCROLLBAR_NEVER );

    widget_get_preferred_size( scrollpanel, &tmp );

    terminal_widget = create_terminal_widget( terminal );
    widget_add( scrollpanel, terminal_widget, NULL );
    widget_dec_ref( terminal_widget );

    /* Calculate the size of the terminal window */

    terminal_widget_get_character_size( terminal_widget, &size.x, &size.y );

    size.x *= 80;
    size.y *= 25;

    point_add( &size, &tmp );

    window = create_window( "Terminal", &point, &size, WINDOW_NONE );

    bitmap_t* icon = bitmap_load_from_file( "/application/terminal/images/terminal.png" );
    window_set_icon( window, icon );
    bitmap_dec_ref( icon );

    /* Create a window */

    widget_t* container = window_get_container( window );

    layout_t* layout = create_border_layout();
    panel_set_layout( container, layout );
    layout_dec_ref( layout );

    widget_add( container, scrollpanel, BRD_CENTER );
    widget_dec_ref( scrollpanel );

    return 0;
}

int main( int argc, char** argv ) {
    if ( application_init() != 0 ) {
        dbprintf( "Failed to initialize taskbar application!\n" );
        return EXIT_FAILURE;
    }

    initialize_terminal();
    initialize_gui();
    initialize_pty();

    /* Show the window */

    window_show( window );

    application_run();

    return EXIT_SUCCESS;
}
