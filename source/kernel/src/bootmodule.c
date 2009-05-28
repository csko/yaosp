/* Boot module management
 *
 * Copyright (c) 2008, 2009 Zoltan Kovacs
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

#include <bootmodule.h>
#include <macros.h>
#include <config.h>
#include <kernel.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

static int bootmodule_count = 0;
static bootmodule_t bootmodules[ MAX_BOOTMODULE_COUNT ];

int get_bootmodule_count( void ) {
    return bootmodule_count;
}

bootmodule_t* get_bootmodule_at( int index ) {
    if ( ( index < 0 ) || ( index >= MAX_BOOTMODULE_COUNT ) ) {
        return NULL;
    }

    return &bootmodules[ index ];
}

static int bm_read_module( void* private, void* data, off_t offset, int size ) {
    bootmodule_t* bootmodule;

    bootmodule = ( bootmodule_t* )private;

    if ( offset + size > bootmodule->size ) {
        size = bootmodule->size - offset;
    }

    if ( size > 0 ) {
        memcpy( data, ( char* )bootmodule->address + offset, size );
    }

    return size;
}

static char* bm_get_name( void* private ) {
    bootmodule_t* bootmodule;

    bootmodule = ( bootmodule_t* )private;

    return bootmodule->name;
}

binary_loader_t* get_bootmodule_loader( int index ) {
    binary_loader_t* loader;

    if ( ( index < 0 ) || ( index >= MAX_BOOTMODULE_COUNT ) ) {
        return NULL;
    }

    loader = ( binary_loader_t* )kmalloc( sizeof( binary_loader_t ) );

    if ( __unlikely( loader == NULL ) ) {
        return NULL;
    }

    loader->private = ( void* )&bootmodules[ index ];
    loader->read = bm_read_module;
    loader->get_name = bm_get_name;
    loader->get_fd = NULL;

    return loader;
}

void put_bootmodule_loader( binary_loader_t* loader ) {
    kfree( loader );
}

__init int init_bootmodules( multiboot_header_t* header ) {
    int i;
    multiboot_module_t* modules;

    bootmodule_count = MIN( MAX_BOOTMODULE_COUNT, header->module_count );
    modules = ( multiboot_module_t* )header->first_module;

    for ( i = 0; i < bootmodule_count; i++ ) {
        bootmodules[ i ].address = ( void* )modules[ i ].start;
        bootmodules[ i ].size = ( size_t )( modules[ i ].end - modules[ i ].start );

        /* Extract the bootmodule name from the parameters */

        if ( modules[ i ].parameters == NULL ) {
            memcpy( bootmodules[ i ].name, "unknown", 8 );
        } else {
            char* end;
            char* start;
            size_t length;

            end = strchr( modules[ i ].parameters, ' ' );

            if ( end != NULL ) {
                *end = 0;
            }

            start = strrchr( modules[ i ].parameters, '/' );

            if ( start == NULL ) {
                start = modules[ i ].parameters;
            } else {
                start++;
            }

            length = MIN( BOOTMODULE_NAME_LENGTH - 1, strlen( start ) );

            memcpy(
                bootmodules[ i ].name,
                start,
                length
            );

            bootmodules[ i ].name[ length ] = 0;
        }
    }

    return 0;
}
