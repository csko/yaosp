/* Module loader and manager
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

#include <module.h>
#include <console.h>
#include <errno.h>
#include <semaphore.h>
#include <mm/kmalloc.h>
#include <lib/string.h>

module_loader_t* module_loader;

static int module_id_counter = 0;
static hashtable_t module_table;
static semaphore_id module_lock;

module_t* create_module( const char* name ) {
    module_t* module;

    module = ( module_t* )kmalloc( sizeof( module_t ) );

    if ( module == NULL ) {
        return NULL;
    }

    memset( module, 0, sizeof( module_t ) );

    module->name = strdup( name );

    if ( module->name == NULL ) {
        kfree( module );
        return NULL;
    }

    return module;
}

int read_module_data( module_reader_t* reader, void* buffer, off_t offset, int size ) {
    return reader->read( reader->private, buffer, offset, size );
}

size_t get_module_size( module_reader_t* reader ) {
    return reader->get_size( reader->private );
}

char* get_module_name( module_reader_t* reader ) {
    return reader->get_name( reader->private );
}

module_id load_module( module_reader_t* reader ) {
    int id;
    bool found;
    module_t* module;

    if ( module_loader == NULL ) {
        return -EINVAL;
    }

    if ( !module_loader->check( reader ) ) {
        return -EINVAL;
    }

    module = module_loader->load( reader );

    if ( module == NULL ) {
        return -EINVAL;
    }

    found = module_loader->get_symbol( module, "init_module", ( ptr_t* )&module->init );

    if ( !found ) {
        module_loader->free( module );
        /* TODO: delete the module */
        return -EINVAL;
    }

    found = module_loader->get_symbol( module, "destroy_module", ( ptr_t* )&module->destroy );

    if ( !found ) {
        module_loader->free( module );
        /* TODO: delete the module */
        return -EINVAL;
    }

    /* Insert the new module to the table */

    LOCK( module_lock );

    do {
        module->id = module_id_counter++;
    } while ( hashtable_get( &module_table, ( const void* )module->id ) != NULL );

    id = module->id;

    hashtable_add( &module_table, ( void* )module );

    UNLOCK( module_lock );

    return id;
}

int initialize_module( module_id id ) {
    module_t* module;

    LOCK( module_lock );

    module = ( module_t* )hashtable_get( &module_table, ( const void* )id );

    UNLOCK( module_lock );

    if ( module == NULL ) {
        return -EINVAL;
    }

    kprintf( "Initializing module: %d\n", id );

    return module->init();
}

void set_module_loader( module_loader_t* loader ) {
    module_loader = loader;
    kprintf( "Using %s module loader\n", module_loader->name );
}

static void* module_key( hashitem_t* item ) {
    module_t* module;

    module = ( module_t* )item;

    return ( void* )module->id;
}

static uint32_t module_hash( const void* key ) {
    return ( uint32_t )key;
}

static bool module_compare( const void* key1, const void* key2 ) {
    return ( key1 == key2 );
}

int init_module_loader( void ) {
    int error;

    module_loader = NULL;

    error = init_hashtable(
        &module_table,
        32,
        module_key,
        module_hash,
        module_compare
    );

    if ( error < 0 ) {
        return error;
    }

    module_lock = create_semaphore( "module lock", SEMAPHORE_BINARY, 0, 1 );

    if ( module_lock < 0 ) {
        destroy_hashtable( &module_table );
        return module_lock;
    }

    return 0;
}
