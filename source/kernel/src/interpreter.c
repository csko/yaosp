/* Interpreter application loader
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

#include <errno.h>
#include <loader.h>
#include <console.h>
#include <kernel.h>
#include <macros.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>
#include <lib/string.h>

static bool interpreter_check( binary_loader_t* loader ) {
    int error;
    char buf[ 3 ];

    error = loader->read(
        loader->private,
        buf,
        0,
        3
    );

    if ( __unlikely( error != 3 ) ) {
        return false;
    }

    return ( memcmp( buf, "#! ", 3 ) == 0 );
}

static int interpreter_execute( binary_loader_t* loader, const char* name, char** argv, char** envp ) {
    int i;
    int data;
    int argc;
    off_t pos;
    char* end;
    char** new_argv;
    char interpreter[ 128 ];

    pos = 3;

    data = loader->read(
        loader->private,
        interpreter,
        sizeof( interpreter ) - 1,
        3
    );

    put_app_binary_loader( loader );

    if ( __unlikely( data < 2 ) ) {
        return -EIO;
    }

    interpreter[ data ] = 0;

    end = strchr( interpreter, '\n' );

    if ( end != NULL ) {
        *end = 0;
    }

    for ( argc = 0; argv[ argc ] != NULL; argc++ ) ;

    new_argv = ( char** )kmalloc( sizeof( char* ) * ( argc + 2 ) );

    if ( new_argv == NULL ) {
        return -ENOMEM;
    }

    new_argv[ 0 ] = strrchr( interpreter, '/' );

    if ( new_argv[ 0 ] == NULL ) {
        new_argv[ 0 ] = interpreter;
    } else {
        new_argv[ 0 ]++;
    }

    for ( i = 0; i < argc; i++ ) {
        new_argv[ i + 1 ] = argv[ i ];
    }

    new_argv[ argc + 1 ] = NULL;

    return do_execve( interpreter, new_argv, envp, true );
}

static interpreter_loader_t interpreter_loader = {
    .name = "interpreter",
    .check = interpreter_check,
    .execute = interpreter_execute
};

__init int init_interpreter_loader( void ) {
    int error;

    error = register_interpreter_loader( &interpreter_loader );

    if ( error < 0 ) {
        return error;
    }

    return 0;
}
