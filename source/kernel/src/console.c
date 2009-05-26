/* Console handling functions
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#include <types.h>
#include <console.h>
#include <macros.h>
#include <lib/stdarg.h>
#include <lib/printf.h>

#include <arch/spinlock.h>

#define KERNEL_CONSOLE_SIZE 32768

static int kernel_read_pos = 0;
static int kernel_write_pos = 0;
static int kernel_console_size = 0;
static char kernel_console[ KERNEL_CONSOLE_SIZE ];

static console_t* screen = NULL;
static console_t* debug = NULL;
static console_t* real_screen = NULL;
static spinlock_t console_lock = INIT_SPINLOCK( "console" );

console_t* console_get_real_screen( void ) {
    return real_screen;
}

int console_set_screen( console_t* console ) {
    if ( ( console != NULL ) && ( console->ops->init != NULL ) ) {
        console->ops->init( console );
    }

    spinlock_disable( &console_lock );

    screen = console;

    if ( real_screen == NULL ) {
        real_screen = console;
    }

    spinunlock_enable( &console_lock );

    return 0;
}

int console_switch_screen( console_t* new_console, console_t** old_console ) {
    if ( ( new_console != NULL ) && ( new_console->ops->init != NULL ) ) {
        new_console->ops->init( new_console );
    }

    spinlock_disable( &console_lock );

    *old_console = screen;
    screen = new_console;

    spinunlock_enable( &console_lock );

    return 0;
}

int console_set_debug( console_t* console ) {
    if ( ( console != NULL ) && ( console->ops->init != NULL ) ) {
        console->ops->init( console );
    }

    spinlock_disable( &console_lock );

    debug = console;

    spinunlock_enable( &console_lock );

    return 0;
}

static int kprintf_helper( void* data, char c ) {
    if ( screen != NULL ) {
        screen->ops->putchar( screen, c );
    }

    if ( debug != NULL ) {
        debug->ops->putchar( debug, c );
    }

    if ( kernel_console_size < KERNEL_CONSOLE_SIZE ) {
        kernel_console[ kernel_write_pos ] = c;

        kernel_write_pos = ( kernel_write_pos + 1 ) % KERNEL_CONSOLE_SIZE;
        kernel_console_size++;
    }

    return 0;
}

int kprintf( const char* format, ... ) {
    va_list args;

    spinlock_disable( &console_lock );

    /* Print the text */

    va_start( args, format );
    do_printf( kprintf_helper, NULL, format, args );
    va_end( args );

    /* Flush the consoles */

    if ( ( screen != NULL ) && ( screen->ops->flush != NULL ) ) {
        screen->ops->flush( screen );
    }

    if ( ( debug != NULL ) && ( debug->ops->flush != NULL ) ) {
        debug->ops->flush( debug );
    }

    spinunlock_enable( &console_lock );

    return 0;
}

int kvprintf( const char* format, va_list args ) {
    spinlock_disable( &console_lock );

    /* Print the text */

    do_printf( kprintf_helper, NULL, format, args );

    /* Flush the consoles */

    if ( ( screen != NULL ) && ( screen->ops->flush != NULL ) ) {
        screen->ops->flush( screen );
    }

    if ( ( debug != NULL ) && ( debug->ops->flush != NULL ) ) {
        debug->ops->flush( debug );
    }

    spinunlock_enable( &console_lock );

    return 0;
}

static int dprintf_helper( void* data, char c ) {
    if ( debug != NULL ) {
        debug->ops->putchar( debug, c );
    }

    return 0;
}

int dprintf( const char* format, ... ) {
    va_list args;

    spinlock_disable( &console_lock );

    /* Print the text */

    va_start( args, format );
    do_printf( dprintf_helper, NULL, format, args );
    va_end( args );

    /* Flush the debug console */

    if ( ( debug != NULL ) && ( debug->ops->flush != NULL ) ) {
        debug->ops->flush( debug );
    }

    spinunlock_enable( &console_lock );

    return 0;
}

int dprintf_unlocked( const char* format, ... ) {
    va_list args;

    /* Print the text */

    va_start( args, format );
    do_printf( dprintf_helper, NULL, format, args );
    va_end( args );

    /* Flush the debug console */

    if ( ( debug != NULL ) && ( debug->ops->flush != NULL ) ) {
        debug->ops->flush( debug );
    }

    return 0;
}

int kernel_console_read( char* buffer, int size ) {
    int ret;
    int to_read;

    spinlock_disable( &console_lock );

    to_read = MIN( kernel_console_size, size );
    ret = to_read;

    while ( to_read > 0 ) {
        *buffer++ = kernel_console[ kernel_read_pos ];

        kernel_read_pos = ( kernel_read_pos + 1 ) % KERNEL_CONSOLE_SIZE;
        kernel_console_size--;
        to_read--;
    }

    spinunlock_enable( &console_lock );

    return ret;
}
