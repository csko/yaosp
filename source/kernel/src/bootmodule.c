/* Boot module management
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

    memcpy( data, ( char* )bootmodule->address + offset, size );

    return size;
}

static size_t bm_get_size( void* private ) {
    bootmodule_t* bootmodule;

    bootmodule = ( bootmodule_t* )private;

    return bootmodule->size;
}

module_reader_t* get_bootmodule_reader( int index ) {
    module_reader_t* reader;

    if ( ( index < 0 ) || ( index >= MAX_BOOTMODULE_COUNT ) ) {
        return NULL;
    }

    reader = ( module_reader_t* )kmalloc( sizeof( module_reader_t ) );

    if ( reader == NULL ) {
        return NULL;
    }

    reader->private = &bootmodules[ index ];
    reader->read = bm_read_module;
    reader->get_size = bm_get_size;

    return reader;
}

void put_bootmodule_reader( module_reader_t* reader ) {
    kfree( reader );
}

int init_bootmodules( multiboot_header_t* header ) {
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
