/* Entry point of the C library
 *
 * Copyright (c) 2009, 2010 Zoltan Kovacs
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

#include <yaosp/debug.h>

#define MAX_ENV_COUNT 256

extern int main( int argc, char** argv, char** envp );

static int errno;

int* __errno_location( void ) {
    return &errno;
}

char** environ;
int __environ_allocated;

typedef void ctor_t( void );

void __libc_start_main( char** argv, char** envp, uint32_t ctor_count, uint32_t* ctor_list ) {
    int argc;
    int error;
    uint32_t i;

    /* Call global constructors. */

    for ( i = 0; i < ctor_count; i++ ) {
        ctor_t* ctor;

        if ( ctor_list[i] == 0xFFFFFFFF ) {
            continue;
        }

        ctor = ( ctor_t* )ctor_list[i];
        ctor();
    }

    /* Count the number of arguments */

    for ( argc = 0; argv[ argc ] != NULL; argc++ ) ;

    /* Store the env array */

    environ = envp;
    __environ_allocated = 0;

    /* Call the main function of the application */

    error = main( argc, argv, envp );

    /* Exit the process */

    exit( error );
}
