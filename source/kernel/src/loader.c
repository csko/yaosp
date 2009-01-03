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

static int execve( char* path, char** argv, char** envp ) {
    int fd;
    int error;
    application_loader_t* loader;

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

    /* Load the executable with the selected loader */

    error = loader->load( fd );

    close( fd );

    if ( error < 0 ) {
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
