/* Init thread
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

#include <kernel.h>
#include <console.h>
#include <bootmodule.h>
#include <module.h>

static void load_bootmodules( void ) {
    int i;
    int error;
    module_id id;
    int module_count;
    module_reader_t* reader;

    module_count = get_bootmodule_count();

    for ( i = 0; i < module_count; i++ ) {
        reader = get_bootmodule_reader( i );

        if ( reader == NULL ) {
            continue;
        }

        id = load_module( reader );

        put_bootmodule_reader( reader );

        if ( id < 0 ) {
            kprintf( "Failed to load bootmodule at %d\n", i );
            continue;
        }

        error = initialize_module( id );

        if ( error < 0 ) {
            kprintf( "Failed to load module: %d\n", id );
            /* TODO: unload the module */
        }
    }
}

int init_thread( void* arg ) {
    kprintf( "Init thread started!\n" );

    load_bootmodules();

    return 0;
}
