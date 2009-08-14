/* Entry point of the C library
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

#include <yaosp/debug.h>

#define MAX_ENV_COUNT 256

extern int init_malloc( void );
extern int main( int argc, char** argv, char** envp );

int errno;

char** environ;
int __environ_allocated;

void __libc_start_main( char** argv, char** envp ) {
    int argc;
    int error;

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
