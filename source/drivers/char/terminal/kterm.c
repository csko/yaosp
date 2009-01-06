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
#include <semaphore.h>
#include <vfs/vfs.h>
#include <lib/string.h>

#include <arch/spinlock.h>

#include "kterm.h"
#include "terminal.h"

#define KTERM_BUFSIZE 4096

static int kterm_tty;

static size_t size;
static size_t read_pos;
static size_t write_pos;
static char kterm_buffer[ KTERM_BUFSIZE ];
static semaphore_id kterm_sync;
static spinlock_t kterm_lock = INIT_SPINLOCK;
static thread_id kterm_flusher;

static void kterm_putchar( console_t* console, char c ) {
    spinlock_disable( &kterm_lock );

    if ( size < KTERM_BUFSIZE ) {
        kterm_buffer[ write_pos ] = c;
        write_pos = ( write_pos + 1 ) % KTERM_BUFSIZE;
        size++;
    }

    spinunlock_enable( &kterm_lock );
}

static void kterm_flush( console_t* console ) {
    UNLOCK( kterm_sync );
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
    while ( 1 ) {
        LOCK( kterm_sync );

        spinlock_disable( &kterm_lock );

        while ( size > 0 ) {
            pwrite( kterm_tty, &kterm_buffer[ read_pos ], 1, 0 );
            read_pos = ( read_pos + 1 ) % KTERM_BUFSIZE;
            size--;
        }

        spinunlock_enable( &kterm_lock );
    }

    return 0;
}

int init_kernel_terminal( void ) {
    char path[ 128 ];
    terminal_t* kterm;

    /* Use the last virtual terminal as the kernel output */

    kterm = terminals[ MAX_TERMINAL_COUNT - 1 ];

    kterm->flags &= ~TERMINAL_ACCEPTS_USER_INPUT;

    /* Open the slave side of the pseudo terminal */

    snprintf( path, sizeof( path ), "/device/pty/tty%d", MAX_TERMINAL_COUNT - 1 );

    kterm_tty = open( path, O_WRONLY );

    if ( kterm_tty < 0 ) {
        kprintf( "Terminal: Failed to open slave tty for kernel!\n" );
        return kterm_tty;
    }

    /* Initialize kterm buffer */

    size = 0;
    read_pos = 0;
    write_pos = 0;

    kterm_sync = create_semaphore( "kterm sync", SEMAPHORE_COUNTING, 0, 0 );

    if ( kterm_sync < 0 ) {
        close( kterm_tty );
        return kterm_sync;
    }

    /* Start kterm buffer flushed */

    kterm_flusher = create_kernel_thread( "kterm flusher", kterm_flusher_thread, NULL );

    if ( kterm_flusher < 0 ) {
        close( kterm_tty );
        delete_semaphore( kterm_sync );
        return kterm_flusher;
    }

    wake_up_thread( kterm_flusher );

    /* Set our conosle as the screen */

    console_set_screen( &kterm_console );

    return 0;
}
