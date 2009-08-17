/* Kernel debugger
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
#include <debug.h>
#include <thread.h>
#include <config.h>
#include <lib/stdarg.h>
#include <lib/printf.h>
#include <lib/string.h>

#ifdef ENABLE_DEBUGGER

static int dbg_running;
static console_t* dbg_screen = NULL;

static char dbg_line_buffer[ 64 ];

static bool dbg_scroll_mode = false;
static int dbg_scroll_line_count = 0;

static int dbg_show_help( const char* params );
static int dbg_exit( const char* params );

static dbg_command_t debugger_commands[] = {
    { "list-threads",      dbg_list_threads,           "List threads" },
    { "show-thread-info",  dbg_show_thread_info,       "Show the informations of the specified thread" },
    { "trace-thread",      dbg_trace_thread,           "Prints the backtrace of the selected thread" },
    { "help",              dbg_show_help,              "Displays this message :)" },
    { "exit",              dbg_exit,                   "Exit from the kernel debugger" },
    { NULL, NULL }
};

static int dbg_printf_helper( void* data, char c ) {
    if ( dbg_screen != NULL ) {
        dbg_screen->ops->putchar( dbg_screen, c );
    }

    if ( dbg_scroll_mode ) {
        if ( c == '\n' ) {
            if ( ++dbg_scroll_line_count == 24 ) {
                dbg_printf( "Press any key to continue." );

                while ( arch_dbg_get_character() == 0 ) { }

                dbg_printf( "\n" );

                dbg_scroll_line_count = 0;
            }
        }
    }


    return 0;
}

int dbg_set_scroll_mode( bool scroll_mode ) {
    if ( scroll_mode ) {
        dbg_scroll_line_count = 0;
    }

    dbg_scroll_mode = scroll_mode;

    return 0;
}

int dbg_printf( const char* format, ... ) {
    va_list args;

    va_start( args, format );
    do_printf( dbg_printf_helper, NULL, format, args );
    va_end( args );

    return 0;
}

static int dbg_show_help( const char* params ) {
    int i;

    dbg_set_scroll_mode( true );

    dbg_printf( "Debugger commands:\n" );

    for ( i = 0; debugger_commands[ i ].command != NULL; i++ ) {
        dbg_printf( "  %s - %s\n", debugger_commands[ i ].command, debugger_commands[ i ].help );
    }

    dbg_set_scroll_mode( false );

    return 0;
}

static int dbg_exit( const char* params ) {
    dbg_running = 0;

    return 0;
}

static char* dbg_get_line( const char* prompt ) {
    char c;
    int line_pos;

    if ( prompt != NULL ) {
        dbg_printf( "%s", prompt );
    }

    line_pos = 0;

    while ( 1 ) {
        c = arch_dbg_get_character();

        switch ( c ) {
            case '\r' :
            case '\n' :
                dbg_line_buffer[ line_pos ] = 0;

                dbg_printf( "\n" );

                return dbg_line_buffer;

            case 0 :
                break;

            case '\b' :
                if ( line_pos > 0 ) {
                    line_pos--;

                    dbg_printf( "\b \b" );
                }

                break;

            default :
                if ( line_pos < ( sizeof( dbg_line_buffer ) - 1 ) ) {
                    dbg_printf( "%c", c );

                    dbg_line_buffer[ line_pos++ ] = c;
                }

                break;
        }
    }
}

int start_kernel_debugger( void ) {
    char* command;

    /* Initialize the screen */

    dbg_running = 1;
    dbg_screen = console_get_real_screen();

    dbg_screen->ops->clear( dbg_screen );
    dbg_printf( "Welcome to the kernel debugger :)\n\n" );

    /* Start the mainloop of the debugger */

    while ( dbg_running ) {
        int i;
        char* params;

        command = dbg_get_line( "> " );

        params = strchr( command, ' ' );

        if ( params != NULL ) {
            *params++ = 0;
        }

        for ( i = 0; debugger_commands[ i ].command != NULL; i++ ) {
            if ( strcmp( debugger_commands[ i ].command, command ) == 0 ) {
                debugger_commands[ i ].callback( params );

                break;
            }
        }
    }

    return 0;
}

#endif /* ENABLE_DEBUGGER */
