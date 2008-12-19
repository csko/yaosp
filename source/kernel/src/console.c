/* Console handling functions
 *
 * Copyright (c) 2008 Zoltan Kovacs
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
#include <lib/stdarg.h>
#include <lib/printf.h>

#include <arch/spinlock.h>

static console_t* screen = NULL;
static spinlock_t console_lock = INIT_SPINLOCK;

int console_set_screen( console_t* console ) {
    screen = console;
    return 0;
}

static int kprintf_helper( void* data, char c ) {
    if ( screen != NULL ) {
        screen->ops->putchar( screen, c );
    }

    return 0;
}

int kprintf( const char* format, ... ) {
    va_list args;

    spinlock_disable( &console_lock );

    va_start( args, format );
    do_printf( kprintf_helper, NULL, format, args );
    va_end( args );

    spinunlock_enable( &console_lock );

    return 0;
}
