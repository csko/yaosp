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
#include <kernel.h>
#include <macros.h>
#include <mm/context.h>
#include <mm/kmalloc.h>
#include <sched/scheduler.h>
#include <vfs/vfs.h>

#define USER_STACK_PAGES ( USER_STACK_SIZE / PAGE_SIZE )

typedef struct app_loader_private {
    int fd;
} app_loader_private_t;

static application_loader_t* application_loaders;
static interpreter_loader_t* interpreter_loaders;

static int app_loader_read( void* _private, void* buffer, off_t offset, int size ) {
    app_loader_private_t* private;

    private = ( app_loader_private_t* )_private;

    return sys_pread(
        private->fd,
        buffer,
        size,
        &offset
    );
}

static int app_loader_get_fd( void* _private ) {
    app_loader_private_t* private;

    private = ( app_loader_private_t* )_private;

    return private->fd;
}

binary_loader_t* get_app_binary_loader( int fd ) {
    binary_loader_t* loader;
    app_loader_private_t* private;

    loader = ( binary_loader_t* )kmalloc( sizeof( binary_loader_t ) + sizeof( app_loader_private_t ) );

    if ( loader == NULL ) {
        return NULL;
    }

    private = ( app_loader_private_t* )( loader + 1 );

    private->fd = fd;

    loader->private = ( void* )private;
    loader->read = app_loader_read;
    loader->get_name = NULL;
    loader->get_fd = app_loader_get_fd;

    return loader;
}

void put_app_binary_loader( binary_loader_t* loader ) {
    kfree( loader );
}

static application_loader_t* find_application_loader( binary_loader_t* binary_loader ) {
    application_loader_t* loader;

    loader = application_loaders;

    while ( loader != NULL ) {
        if ( loader->check( binary_loader ) ) {
            break;
        }

        loader = loader->next;
    }

    return loader;
}

static interpreter_loader_t* find_interpreter_loader( binary_loader_t* binary_loader ) {
    interpreter_loader_t* loader;

    loader = interpreter_loaders;

    while ( loader != NULL ) {
        if ( loader->check( binary_loader ) ) {
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

    if ( __unlikely( old_array == NULL ) ) {
        old_size = 0;
        new_array = NULL;
    } else {
        for ( old_size = 0; old_array[ old_size ] != NULL; old_size++ ) ;

        if ( old_size == 0 ) {
            new_array = NULL;
            goto out;
        }

        new_array = ( char** )kmalloc( sizeof( char* ) * old_size );

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
    }

 out:
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

int get_application_symbol_info( thread_t* thread, ptr_t address, symbol_info_t* info ) {
    application_loader_t* loader;

    if ( thread == NULL ) {
        return -EINVAL;
    }

    loader = thread->process->loader;

    if ( loader == NULL ) {
        return -EINVAL;
    }

    return loader->get_symbol_info( thread, address, info );
}

int do_execve( char* path, char** argv, char** envp, bool free_argv ) {
    int i;
    int fd;
    int error;
    char* new_name;
    uint8_t* stack;
    thread_t* thread;
    application_loader_t* loader;
    interpreter_loader_t* interpreter_loader;

    int argc;
    char** cloned_argv;
    char** user_argv;
    int envc;
    char** cloned_envv;
    char** user_envv;

    struct sigaction* handler;
    binary_loader_t* binary_loader;

    /* Open the file */

    fd = sys_open( path, O_RDONLY );

    if ( fd < 0 ) {
        error = fd;
        goto _error1;
    }

    binary_loader = get_app_binary_loader( fd );

    /* Find the proper loader for it */

    interpreter_loader = find_interpreter_loader( binary_loader );

    if ( interpreter_loader != NULL ) {
        return interpreter_loader->execute( binary_loader, path, argv, envp );
    }

    loader = find_application_loader( binary_loader );

    if ( loader == NULL ) {
        error = -EINVAL;
        goto _error2;
    }

    thread = current_thread();

    /* Clone the argv and envp */

    error = clone_param_array( argv, &cloned_argv, &argc );

    if ( error < 0 ) {
        goto _error2;
    }

    error = clone_param_array( envp, &cloned_envv, &envc );

    if ( error < 0 ) {
        goto _error3;
    }

    if ( free_argv ) {
        kfree( argv );
    }

    /* Rename the process and the thread */

    new_name = strrchr( path, '/' );

    if ( new_name == NULL ) {
        new_name = path;
    } else {
        new_name++;
    }

    scheduler_lock();
    rename_process( thread->process, new_name );
    rename_thread( thread, "main" );
    scheduler_unlock();

    /* Empty the memory context of the process */

    memory_context_delete_regions( thread->process->memory_context );

    thread->process->heap_region = NULL;

    /* Empty the locking context of the process */

    lock_context_make_empty( thread->process->lock_context );
    /* TODO: close those files that have close_on_exec flag turned on! */

    /* Set default signal handling values */

    for ( i = 0, handler = &thread->signal_handlers[ 0 ]; i < _NSIG - 1; i++, handler++ ) {
        handler->sa_handler = SIG_DFL;
        handler->sa_flags = 0;
    }

    thread->signal_handlers[ SIGCHLD - 1 ].sa_handler = SIG_IGN;

    /* Load the executable with the selected loader */

    error = loader->load( binary_loader );

    put_app_binary_loader( binary_loader );
    sys_close( fd );

    if ( error < 0 ) {
        goto error1;
    }

    /* Create stack for the userspace thread */

    thread->user_stack_region = memory_region_create(
        "stack",
        USER_STACK_PAGES * PAGE_SIZE,
        REGION_READ | REGION_WRITE | REGION_STACK
    );

    if ( thread->user_stack_region == NULL ) {
        goto error2;
    }

    memory_region_alloc_pages( thread->user_stack_region ); /* todo: check return value */

    /* Copy argv and envp item values to the user */

    user_argv = ( char** )kmalloc( sizeof( char* ) * ( argc + 1 ) );

    if ( user_argv == NULL ) {
        goto error3;
    }

    user_envv = ( char** )kmalloc( sizeof( char* ) * ( envc + 1 ) );

    if ( user_envv == NULL ) {
        goto error4;
    }

    stack = ( uint8_t* )thread->user_stack_region->address;
    stack += ( USER_STACK_PAGES * PAGE_SIZE );

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

    kfree( user_argv );
    kfree( user_envv );

    /* Push argv and envp to the stack */

    stack -= sizeof( void* );
    *( ( void** )stack ) = ( void* )envp;
    stack -= sizeof( void* );
    *( ( void** )stack ) = ( void* )argv;

    /* Save the modified stack pointer in the thread structure */

    thread->user_stack_end = ( void* )stack;
    thread->process->loader = loader;

    /* Start the executable */

    error = loader->execute();

    if ( error < 0 ) {
        /* NOTE: We can jump to the error3 label here because kfree() has been
                 called on user_argv and user_envv before. */

        goto error3;
    }

    return 0;

/* Cleanup process before the memory context is destroyed */

_error3:
    free_param_array( cloned_argv, argc );

_error2:
    sys_close( fd );

_error1:
    return error;

/* Cleanup process after the memory context is destroyed */

error4:
    kfree( user_argv );

error3:
    memory_region_put( thread->user_stack_region );

error2:
    /* TODO: destroy loaded stuff */

error1:
    thread->user_stack_region = NULL;

    kprintf( ERROR, "Failed to execute %s.\n", thread->process->name );

    thread_exit( 0 );

    /* This is just here to keep GCC happy, but never reached. */

    return 0;
}

int sys_execve( char* path, char** argv, char** envp ) {
    return do_execve( path, argv, envp, false );
}

int register_application_loader( application_loader_t* loader ) {
    loader->next = application_loaders;
    application_loaders = loader;

    kprintf( INFO, "Registered application loader: %s.\n", loader->name );

    return 0;
}

int register_interpreter_loader( interpreter_loader_t* loader ) {
    loader->next = interpreter_loaders;
    interpreter_loaders = loader;

    kprintf( INFO, "Registered interpreter loader: %s.\n", loader->name );

    return 0;
}

__init int init_application_loader( void ) {
    application_loaders = NULL;
    interpreter_loaders = NULL;

    return 0;
}
