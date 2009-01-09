/* Application loader
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

#include <loader.h>
#include <errno.h>
#include <console.h>
#include <smp.h>
#include <mm/context.h>
#include <mm/kmalloc.h>
#include <vfs/vfs.h>

static application_loader_t* loaders;

static application_loader_t* find_application_loader( int fd ) {
    application_loader_t* loader;

    loader = loaders;

    while ( loader != NULL ) {
        if ( loader->check( fd ) ) {
            break;
        }

        loader = loader->next;
    }

    return loader;
}

static int clone_param_array( char** old_array, char*** _new_array, int* new_size ) {
    int i;
    int old_size;
    char** new_array;

    if ( old_array == NULL ) {
        old_size = 0;
    } else {
        for ( old_size = 0; old_array[ old_size ] != NULL; old_size++ ) ;
    }

    new_array = ( char** )kmalloc( sizeof( char* ) * ( old_size + 1 ) );

    if ( new_array == NULL ) {
        return -ENOMEM;
    }

    for ( i = 0; i < old_size; i++ ) {
        if ( old_array[ i ] == NULL ) {
            new_array[ i ] = NULL;
        } else {
            size_t len;

            len = strlen( old_array[ i ] );

            new_array[ i ] = ( char* )kmalloc( len + 1 );

            if ( new_array[ i ] == NULL ) {
                return -ENOMEM;
            }

            memcpy( new_array[ i ], old_array[ i ], len + 1 );
        }
    }

    new_array[ old_size ] = NULL;

    *_new_array = new_array;
    *new_size = old_size;

    return 0;
}

static int free_param_array( char** array, int count ) {
    int i;

    for ( i = 0; i < count; i++ ) {
        kfree( array[ i ] );
    }

    kfree( array );

    return 0;
}

static uint8_t* copy_param_array_to_user( char** array, char** user_array, int count, uint8_t* stack ) {
    int i;
    size_t length;

    for ( i = 0; i < count; i++ ) {
        if ( array[ i ] == NULL ) {
            user_array[ i ] = NULL;
        } else {
            length = strlen( array[ i ] );
            stack -= ( length + 1 );
            user_array[ i ] = ( char* )stack;

            memcpy( stack, array[ i ], length + 1 );
        }
    }

    return stack;
}

static int execve( char* path, char** argv, char** envp ) {
    int fd;
    int error;
    uint8_t* stack;
    application_loader_t* loader;

    int argc;
    char** cloned_argv;
    char** user_argv;
    int envc;
    char** cloned_envv;
    char** user_envv;

    /* Open the file */

    fd = open( path, 0 );

    if ( fd < 0 ) {
        return fd;
    }

    /* Find the proper loader for it */

    loader = find_application_loader( fd );

    if ( loader == NULL ) {
        close( fd );
        return -ENOEXEC;
    }

    /* Clone the argv and envp */

    error = clone_param_array( argv, &cloned_argv, &argc );

    if ( error < 0 ) {
        close( fd );
        return error;
    }

    error = clone_param_array( envp, &cloned_envv, &envc );

    if ( error < 0 ) {
        /* TODO: free cloned argv */
        close( fd );
        return error;
    }

    user_argv = ( char** )kmalloc( sizeof( char* ) * ( argc + 1 ) );
    user_envv = ( char** )kmalloc( sizeof( char* ) * ( envc + 1 ) );

    memory_context_delete_regions( current_process()->memory_context, true );

    /* Load the executable with the selected loader */

    error = loader->load( fd );

    close( fd );

    if ( error < 0 ) {
        return error;
    }

    /* Copy argv and envp item values to the user */

    stack = ( uint8_t* )current_thread()->user_stack_end;

    stack = copy_param_array_to_user( cloned_argv, user_argv, argc, stack );
    stack = copy_param_array_to_user( cloned_envv, user_envv, envc, stack );

    free_param_array( cloned_argv, argc );
    free_param_array( cloned_envv, envc );

    user_argv[ argc ] = NULL;
    user_envv[ envc ] = NULL;

    /* Copy argv and envp arrays to the user */

    stack -= ( sizeof( char* ) * ( argc + 1 ) );
    argv = ( char** )stack;
    stack -= ( sizeof( char* ) * ( envc + 1 ) );
    envp = ( char** )stack;

    memcpy( argv, user_argv, sizeof( char* ) * ( argc + 1 ) );
    memcpy( envp, user_envv, sizeof( char* ) * ( envc + 1 ) );

    /* Push argv and envp to the stack */

    stack -= sizeof( void* );
    *( ( void** )stack ) = ( void* )envp;
    stack -= sizeof( void* );
    *( ( void** )stack ) = ( void* )argv;

    /* Save the modified stack pointer in the thread structure */

    current_thread()->user_stack_end = ( void* )stack;

    /* Start the executable */

    error = loader->execute();

    if ( error < 0 ) {
        /* TODO: unload the executable */
        return error;
    }

    return 0;
}

int sys_execve( char* path, char** argv, char** envp ) {
    return execve( path, argv, envp );
}

int register_application_loader( application_loader_t* loader ) {
    loader->next = loaders;
    loaders = loader;

    kprintf( "%s application loader registered.\n", loader->name );

    return 0;
}

int init_application_loader( void ) {
    loaders = NULL;

    return 0;
}
