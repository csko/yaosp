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
#include <thread.h>
#include <macros.h>
#include <lock/semaphore.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/spinlock.h>

#include "kterm.h"
#include "terminal.h"

#define KTERM_BUFSIZE 32768

static int kterm_tty;

static size_t size;
static size_t read_pos;
static size_t write_pos;
static char kterm_buffer[ KTERM_BUFSIZE ];
static lock_id kterm_sync;
static spinlock_t kterm_lock = INIT_SPINLOCK( "kernel terminal" );
static thread_id kterm_flusher;

static void kterm_putchar( console_t* console, char c ) {
    spinlock_disable( &kterm_lock );

    if ( __likely( size < KTERM_BUFSIZE ) ) {
        kterm_buffer[ write_pos ] = c;

        write_pos = ( write_pos + 1 ) % KTERM_BUFSIZE;
        size++;
    }

    spinunlock_enable( &kterm_lock );
}

static void kterm_flush( console_t* console ) {
    semaphore_unlock( kterm_sync, 1 );
}

static console_operations_t kterm_console_ops = {
    .init = NULL,
    .clear = NULL,
    .putchar = kterm_putchar,
    .gotoxy = NULL,
    .flush = kterm_flush
};

static console_t kterm_console = {
    .width = 80,
    .height = 25,
    .ops = &kterm_console_ops
};

static int kterm_flusher_thread( void* arg ) {
    bool more_data = false;
    int cnt;
    char tmp[ 128 ];

    while ( 1 ) {
        if ( !more_data ) {
            semaphore_lock( kterm_sync, 1, LOCK_IGNORE_SIGNAL );
        }

        spinlock_disable( &kterm_lock );

        cnt = 0;

        while ( ( size > 0 ) && ( cnt < sizeof( tmp ) ) ) {
            tmp[ cnt++ ] = kterm_buffer[ read_pos ];
            read_pos = ( read_pos + 1 ) % KTERM_BUFSIZE;
            size--;
        }

        more_data = ( size > 0 );

        spinunlock_enable( &kterm_lock );

        if ( cnt > 0 ) {
            pwrite( kterm_tty, tmp, cnt, 0 );
        }
    }

    return 0;
}

int init_kernel_terminal( void ) {
    int data;
    char buf[ 256 ];
    terminal_t* kterm;

    /* Use the last virtual terminal as the kernel output */

    kterm = terminals[ MAX_TERMINAL_COUNT - 1 ];

    kterm->flags &= ~TERMINAL_ACCEPTS_USER_INPUT;

    /* Open the slave side of the pseudo terminal */

    snprintf( buf, sizeof( buf ), "/device/terminal/tty%d", MAX_TERMINAL_COUNT - 1 );

    kterm_tty = open( buf, O_WRONLY );

    if ( kterm_tty < 0 ) {
        kprintf( ERROR, "Terminal: Failed to open slave tty for kernel!\n" );
        return kterm_tty;
    }

    /* Initialize kterm buffer */

    size = 0;
    read_pos = 0;
    write_pos = 0;

    kterm_sync = semaphore_create( "kterm wait semaphore", 0 );

    if ( kterm_sync < 0 ) {
        close( kterm_tty );
        return kterm_sync;
    }

    /* Start kterm buffer flushed */

    kterm_flusher = create_kernel_thread( "kterm flusher", PRIORITY_NORMAL, kterm_flusher_thread, NULL, 0 );

    if ( kterm_flusher < 0 ) {
        close( kterm_tty );
        semaphore_destroy( kterm_sync );
        return kterm_flusher;
    }

    thread_wake_up( kterm_flusher );

    /* Set our conosle as the screen */

    console_set_screen( &kterm_console );

    /* Copy the buffered kernel output to the terminal buffer */

    while ( ( data = kernel_console_read( buf, sizeof( buf ) ) ) > 0 ) {
        char* c = buf;

        for ( ; data > 0; data--, c++ ) {
            terminal_put_char( kterm, *c );
        }
    }

    /* Make the kernel terminal the active */

    terminal_switch_to( MAX_TERMINAL_COUNT - 1 );

    return 0;
}
