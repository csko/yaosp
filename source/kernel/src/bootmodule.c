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

int bootmodule_count = 0;
bootmodule_t bootmodules[ MAX_BOOTMODULE_COUNT ];

int get_bootmodule_count( void ) {
    return bootmodule_count;
}

bootmodule_t* get_bootmodule_at( int index ) {
    if ( ( index < 0 ) || ( index >= MAX_BOOTMODULE_COUNT ) ) {
        return NULL;
    }

    return &bootmodules[ index ];
}

int init_bootmodules( multiboot_header_t* header ) {
    int i;
    multiboot_module_t* modules;

    bootmodule_count = MIN( MAX_BOOTMODULE_COUNT, header->module_count );
    modules = ( multiboot_module_t* )header->first_module;

    for ( i = 0; i < bootmodule_count; i++ ) {
        bootmodules[ i ].address = ( void* )modules[ i ].start;
        bootmodules[ i ].size = ( size_t )( modules[ i ].end - modules[ i ].start );
    }

    return 0;
}
