/* exit function
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
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

#include <yaosp/syscall.h>
#include <yaosp/syscall_table.h>

typedef void ( *atexit_func_t )( void );

static uint32_t atexit_count = 0;
static atexit_func_t atexit_functions[ ATEXIT_MAX ];

int atexit( void ( *function )( void ) ) {
    if ( atexit_count >= ATEXIT_MAX ) {
        return -1;
    }

    atexit_functions[ atexit_count++ ] = function;

    return 0;
}

void _exit( int status ) {
    /* Flush stdout before exit */

    fflush( stdout );

    syscall1( SYS_exit, status );
}

void exit( int status ) {
    uint32_t i;

    for ( i = 0; i < atexit_count; i++ ) {
        atexit_functions[ i ]();
    }

    _exit( status );
}
